//
// Created by HAIRONG ZHU on 25-2-15.
//

#ifndef FOCKNOB_PRESSURE_SENSOR_H
#define FOCKNOB_PRESSURE_SENSOR_H

#include "driver/gpio.h"
#include "esp_timer.h"


class PressureSensor {
public:
    explicit PressureSensor(gpio_num_t dout_pin, gpio_num_t sck_pin);

    void tare(); // 去皮功能, 更新零点偏移量
    void set_reference_unit(float reference_unit); // 设置校准系数
    [[nodiscard]] bool is_pressed() const; // 按钮是否被按下
    [[nodiscard]] float get_weight() const; // 获取当前重量
    [[nodiscard]] float get_weight_low_pass() const; // 获取当前重量的低通滤波值

private:
    // 私有方法
    [[nodiscard]] long _hx711_read() const; // 读取 HX711 原始数据
    void _scale_loop(); // 定时器回调函数, 用于更新 current_weight_

    // 静态定时器回调函数
    static void _timer_callback_static(void *args);

    // GPIO 引脚
    gpio_num_t dout_pin_;
    gpio_num_t sck_pin_;

    // 校准参数
    float reference_unit_ = 380.0f; // 校准系数, 需要根据实际情况调整
    long zero_offset_long_ = 502100;       // 零点偏移量

    // 当前重量
    long current_weight_long_ = 0;
    float current_weight_low_pass_ = 0.0f;
    float current_weight_ = 0.0f;

    // 定时器句柄
    esp_timer_handle_t scale_timer_{};

    // 日志标签
    static constexpr const char *TAG = "PressureSensor";
};


#endif //FOCKNOB_PRESSURE_SENSOR_H
