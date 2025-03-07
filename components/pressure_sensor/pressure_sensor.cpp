//
// Created by HAIRONG ZHU on 25-2-15.
//

#include "pressure_sensor.h"
#include "project_conf.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"

// 构造函数, 实现 HX711 的初始化和定时器设置
PressureSensor::PressureSensor(gpio_num_t dout_pin, gpio_num_t sck_pin)
    : dout_pin_(dout_pin), sck_pin_(sck_pin) {
    // 初始化 HX711 的 GPIO

    // 配置 PD_SCK 引脚为输出模式
    gpio_config_t io_cfg = {};
    io_cfg.intr_type = GPIO_INTR_DISABLE; // 禁用中断
    io_cfg.mode = GPIO_MODE_OUTPUT; // 设置为输出模式
    io_cfg.pin_bit_mask = (1ULL << sck_pin_); // 选择 sck_pin_
    io_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE; // 禁用下拉
    io_cfg.pull_up_en = GPIO_PULLUP_DISABLE; // 禁用上拉
    gpio_config(&io_cfg);

    // 配置 DOUT 引脚为输入模式
    io_cfg.mode = GPIO_MODE_INPUT; // 设置为输入模式
    io_cfg.pin_bit_mask = (1ULL << dout_pin_); // 选择 dout_pin_
    gpio_config(&io_cfg);

    // PD_SCK 置低, 确保 HX711 正常工作
    gpio_set_level(sck_pin_, 0);
    vTaskDelay(pdMS_TO_TICKS(100));
    zero_offset_long_ = _hx711_read(); // 初始化零点偏移量

    /*
     * @brief HX711 RATE 数字输入 输出数据速率控制，0: 10Hz; 1: 80Hz
     */
    // 创建并启动定时器, 定期读取重量
    const esp_timer_create_args_t timer_args = {
        .callback = &PressureSensor::_timer_callback_static,
        .arg = this,
        .name = "scale_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &scale_timer_));
    ESP_ERROR_CHECK(esp_timer_start_periodic(scale_timer_, 30000)); // 30ms 读取一次重量
}

// 静态定时器回调函数, 调用实例的 _scale_loop 方法
void PressureSensor::_timer_callback_static(void *args) {
    auto *self = static_cast<PressureSensor *>(args);
    self->_scale_loop();
}

// 定时器循环函数, 读取重量并更新 current_weight_
void PressureSensor::_scale_loop() {
    // 读取原始数据
    long reading = _hx711_read();
    if (reading == -1) {
        ESP_LOGW(TAG, "HX711 temporary error, reading: %ld", reading);
        return;
    }
    // 减去零点偏移
    current_weight_long_ = reading - zero_offset_long_;
    // 计算重量, 单位为克
    current_weight_ = static_cast<float>(current_weight_long_) / reference_unit_;
    // 低通滤波
    current_weight_low_pass_ = current_weight_low_pass_ * 0.6f + current_weight_ * 0.4f;
}

// 读取 HX711 的原始数据
long PressureSensor::_hx711_read() const {
    unsigned long Count = 0;

    // 确保 PD_SCK 置低
    gpio_set_level(sck_pin_, 0);

    // 等待 DOUT 变为低, 表示数据准备好了
    while (gpio_get_level(dout_pin_)) {
        vTaskDelay(pdMS_TO_TICKS(2));
    };

    // 读取 24 位数据
    for (uint8_t i = 0; i < 24; i++) {
        gpio_set_level(sck_pin_, 1);
        // 左移 Count, 腾出一位空间
        Count = Count << 1;
        // PD_SCK 下降沿, 准备读取数据位
        gpio_set_level(sck_pin_, 0);
        // 读取一位数据
        if (gpio_get_level(dout_pin_)) {
            Count++;
        }
    }

    // 第 25 个脉冲, 设置通道和增益, 默认通道 A, 增益 128
    gpio_set_level(sck_pin_, 1);
    gpio_set_level(sck_pin_, 0);

    if (Count & 0x800000) {
        // 用来检测第 24 位是否为 1 (这里的Count为补码表示, 也就是判断正负
        // 判断最高位符号位
        Count |= 0xFF000000; // 负数, 进行符号扩展, 更高的 8 位 (bit 31~24) 全置为 1, 得到了一个 32 位的负数(补码)
    } else {
        Count &= 0x00FFFFFF; // 正数, 保留低 24 位数据
    }

    return static_cast<long>(Count);
}

// 去皮函数, 设置当前读取值为零点偏移量
void PressureSensor::tare() {
    zero_offset_long_ = current_weight_long_;
}

// 设置校准系数
void PressureSensor::set_reference_unit(float reference_unit) {
    reference_unit_ = reference_unit;
    ESP_LOGI(TAG, "Reference unit set to: %.2f", reference_unit_);
}

// 按钮是否被按下
bool PressureSensor::is_pressed() const {
    return current_weight_ > PRESS_THRESHOLD;
}

// 获取当前重量
float PressureSensor::get_weight() const {
    return current_weight_;
}


float PressureSensor::get_weight_low_pass() const {
    return current_weight_low_pass_;
}
