//
// Created by HAIRONG ZHU on 25-1-14.
//

#ifndef FOCKNOB_MOTOR_FOC_DRIVER_H
#define FOCKNOB_MOTOR_FOC_DRIVER_H

#include "iic_as5600.h"
#include "esp_foc.h"
#include "esp_svpwm.h"
#include <esp_timer.h>


class FocDriver {
public:
    FocDriver(gpio_num_t u_gpio,
              gpio_num_t v_gpio,
              gpio_num_t w_gpio,
              gpio_num_t en_gpio,
              AS5600 *as5600,
              int pole_pairs
    );

    void bsp_bridge_driver_enable(bool enable); // 使能电机驱动引脚
    void foc_motor_calibrate();    // 自检电机转向
    void set_dq_out(float Ud, float Uq);    // 设置DQ坐标 (力矩控制)
private:
    float current_ud_ = 0;
    float current_uq_ = 0;

    gpio_num_t en_gpio_{};
    AS5600 *as5600_{};
    int pole_pairs_ = 0;
    float as5600_direction_ = -1.0;  // 1: 正转, -1: 反转
    bool foc_is_enabled_ = false;

    float zero_electric_angle_ = 0;
    foc_dq_coord_t dq_out_{};   // 最大值为FOC_MCPWM_PERIOD / 2
    foc_ab_coord_t ab_out_{};
    foc_uvw_coord_t uvw_out_{};
    int uvw_duty_[3]{};  // 电机PWM占空比
    inverter_handle_t inverter_{};
    esp_timer_handle_t foc_timer{};

    static float _normalize_angle(float angle);   // 角度归一化
    float _get_electrical_angle();   // 获取电机电角度
    static void _timer_callback_static(void *args);   // 定时器回调函数
    void _set_dq_out_loop();   // 设置DQ坐标 (力矩控制) 循环
    void _set_dq_out_exec(float Ud, float Uq, float e_theta_rad);    // 设置DQ坐标 (力矩控制) 执行


};


#endif //FOCKNOB_MOTOR_FOC_DRIVER_H