//
// Created by HAIRONG ZHU on 25-1-14.
//

#include "motor_foc_driver.h"
#include "project_conf.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"

static const char *TAG = "FocDriver";
#define _constrain(amt, low, high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))


FocDriver::FocDriver(gpio_num_t u_gpio,
                     gpio_num_t v_gpio,
                     gpio_num_t w_gpio,
                     gpio_num_t en_gpio,
                     AS5600 *as5600,
                     int pole_pairs) : en_gpio_(en_gpio),
                                       as5600_(as5600),
                                       pole_pairs_(pole_pairs) {
    // 初始化电机驱动，使能引脚, 创建逆变器
    inverter_config_t cfg = {
            .timer_config = {
                    .group_id = 0,
                    .clk_src = MCPWM_TIMER_CLK_SRC_DEFAULT,
                    .resolution_hz = FOC_MCPWM_TIMER_RESOLUTION_HZ,
                    .count_mode = MCPWM_TIMER_COUNT_MODE_UP_DOWN,
                    .period_ticks = FOC_MCPWM_PERIOD,
            },
            .operator_config = {
                    .group_id = 0,
            },
            .compare_config = {
                    .flags = {
                            .update_cmp_on_tez = true,
                    },
            },
            .gen_gpios = {
                    u_gpio,
                    v_gpio,
                    w_gpio,
            },
    };

    ESP_ERROR_CHECK(svpwm_new_inverter(&cfg, &inverter_));   // 新建一个逆变器
    ESP_ERROR_CHECK(svpwm_inverter_start(inverter_, MCPWM_TIMER_START_NO_STOP)); // 启动逆变器
    ESP_LOGI(TAG, "Inverter init OK");

    gpio_config_t drv_en_config = {
            .pin_bit_mask = 1ULL << en_gpio,
            .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&drv_en_config));

    // 创建 foc 主循环定时器
    const esp_timer_create_args_t timer_args = {
            .callback = &_timer_callback_static,
            .arg = this,
            .name = "foc_loop_timer"
    };

    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &foc_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(foc_timer, FOC_CALC_PERIOD));
}

void FocDriver::bsp_bridge_driver_enable(bool enable) {
    foc_is_enabled_ = enable;
    ESP_LOGI(TAG, "%s MOSFET gate", foc_is_enabled_ ? "Enable" : "Disable");
    gpio_set_level((gpio_num_t) en_gpio_, foc_is_enabled_);
}

/**
 * @brief 电机自检函数：开环正转，检测编码器读数变化方向，设置零电角度
 */
void FocDriver::foc_motor_calibrate() {
    if (!foc_is_enabled_) {
        ESP_LOGW(TAG, "Please enable the motor driver first");
        return;
    }
    // 关闭主循环
    esp_timer_stop(foc_timer);

    float test_voltage = FOC_MCPWM_PERIOD / 6.0;  // 设置一个较小的测试电压

    // 第一步: 确定电机的旋转方向
    ESP_LOGI(TAG, "Starting motor direction calibration...");
    float theta = 0;
    float delta_theta = M_TWOPI * 0.01;  // 每次的角度增量
    int test_steps = 30;

    // 施加初始电压并等待稳定
    _set_dq_out_exec(0, test_voltage, 0);  // 开环运行电机到0度
    vTaskDelay(pdMS_TO_TICKS(300));  // 延时300ms
    float initial_angle = as5600_->read_radian_from_sensor_with_no_update();  // 读取编码器角度

    for (int i = 0; i < test_steps; i++) {
        theta += delta_theta;
        _set_dq_out_exec(0, test_voltage, theta);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    float final_angle = as5600_->read_radian_from_sensor_with_no_update();  // 读取最终角度

    // 停止电机
    _set_dq_out_exec(0, 0, theta);
    float angle_difference = final_angle - initial_angle;   // 计算角度差
    if (angle_difference < -M_PI) { // 角度差归一化
        angle_difference += 2 * M_PI;
    } else if (angle_difference > M_PI) {
        angle_difference -= 2 * M_PI;
    }

    // 判断电机旋转方向
    as5600_direction_ = (angle_difference > 0) ? 1.0f : -1.0f;
    ESP_LOGI(TAG, "Motor direction is %s", (as5600_direction_ > 0) ? "1" : "-1");
    // 设置零电角度
    /*
     * @brief 如果设置 Q 的话是不是代表超前 90 度就不是 0 电位角了, 所以需要设置 D 轴 (犯的经典错误(整整搞了一整天!!))
     *        如果不进行零位校准, 就会发生什么 ?
     *        定子磁场与转子磁场不同步: 控制算法认为转子在零位置, 但实际可能在其他位置。这导致定子产生的磁场与转子磁场之间存在相位差。
     *        扭矩产生效率低: 由于相位差, 产生的扭矩不是最大化的, 需要更多的电流来产生相同的扭矩, 导致功率损耗增加。
     *        运行不稳定: 可能出现震荡 、 抖动 、 噪音等问题。
     *        https://www.bilibili.com/video/BV1Bfq6YLEZ2
     */

    // 第二步: 设置零电角度
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_LOGI(TAG, "Setting zero electrical angle...");

    // 施加D轴电压, 使电机定子磁场对准零位
    _set_dq_out_exec(test_voltage, 0, 0);
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 读取编码器角度, 计算零电角度
    zero_electric_angle_ = as5600_->read_radian_from_sensor_with_no_update() * (float) pole_pairs_ * as5600_direction_;
    vTaskDelay(pdMS_TO_TICKS(100));

    // 停止电机
    _set_dq_out_exec(0, 0, 0);
    ESP_LOGI(TAG, "Zero electrical angle is set to %.2f rad", zero_electric_angle_);
    ESP_LOGI(TAG, "Motor direction calibration done.");

    // 重新启动主循环
    esp_timer_start_periodic(foc_timer, FOC_CALC_PERIOD);
}

void FocDriver::set_dq_out(float Ud, float Uq) {
    current_uq_ = Uq * as5600_direction_; // 统一顺时针为正方向
    current_ud_ = Ud;
}

float FocDriver::_normalize_angle(float angle) {
    float a = fmodf(angle, 2.0f * M_PI);   //取余运算可以用于归一化，列出特殊值例子算便知
    return a >= 0 ? a : (a + 2.0f * (float) M_PI);
}

float FocDriver::_get_electrical_angle() {
    return as5600_->read_radian_from_sensor() * (float) pole_pairs_ * as5600_direction_ - zero_electric_angle_;
}

void FocDriver::_timer_callback_static(void *args) {
    auto *self = static_cast<FocDriver *>(args);
    self->_set_dq_out_loop();
}

void FocDriver::_set_dq_out_loop() {    // 定时器循环用于控制电机
    _set_dq_out_exec(current_ud_, current_uq_, _get_electrical_angle());
}

void FocDriver::_set_dq_out_exec(float Ud, float Uq, float e_theta_rad) {
    dq_out_.d = _constrain(Ud, -FOC_MCPWM_PERIOD / 2.0 + 1, FOC_MCPWM_PERIOD / 2.0 - 1);    // 限制Ud的范围
    dq_out_.q = _constrain(Uq, -FOC_MCPWM_PERIOD / 2.0 + 1, FOC_MCPWM_PERIOD / 2.0 - 1);    // 限制Uq的范围

    foc_inverse_park_transform(e_theta_rad, &dq_out_, &ab_out_);
    foc_svpwm_duty_calculate(&ab_out_, &uvw_out_);    // SVPWM计算
    // foc_inverse_clarke_transform(&ab_out_, &uvw_out_); // 克拉克逆变换(SPWM)

    // 设置PWM
    uvw_duty_[0] = int(uvw_out_.u / 2) + FOC_MCPWM_PERIOD / 4;
    uvw_duty_[1] = int(uvw_out_.v / 2) + FOC_MCPWM_PERIOD / 4;
    uvw_duty_[2] = int(uvw_out_.w / 2) + FOC_MCPWM_PERIOD / 4;

    // 使能PWM
    ESP_ERROR_CHECK(svpwm_inverter_set_duty(inverter_, uvw_duty_[0], uvw_duty_[1], uvw_duty_[2]));
}




