#ifndef FOCKNOB_ROTARY_KNOB_H
#define FOCKNOB_ROTARY_KNOB_H

#include "motor_foc_driver.h"
#include <functional>

class RotaryKnob {
public:
    explicit RotaryKnob(FocDriver *focDriver, AS5600 *as5600);

    void stop();    // 停止旋钮
    void attractor(int attractor_num, bool reset_custom_pos, float current_radian);  // 设置棘轮吸附模式
    void attractor_with_rebound(int attractor_num, float left_rad, float right_rad, bool reset_custom_pos, float current_radian, float attractor_kp = 150); // 设置棘轮吸附模式，超出边界后反弹
    void damping(float damping_gain, bool reset_custom_pos, float current_radian);   // 设置阻尼模式 damping_gain 阻尼系数
    void damping_with_rebound(float damping_gain, float left_rad, float right_rad, bool reset_custom_pos, float current_radian); // 设置阻尼模式，超出边界后反弹
    [[nodiscard]] int attractor_get_pos() const;
    [[nodiscard]] float damping_get_pos() const;
    [[nodiscard]] float get_current_radian() const;


private:
    enum class Mode {
        None,
        Attractor,              // 棘轮吸附模式
        AttractorWithRebound,   // 棘轮吸附模式，超出边界后反弹
        Damping,                // 阻尼模式
        DampingWithRebound      // 阻尼模式，超出边界后反弹
    };

    Mode current_mode_ = Mode::None;
    FocDriver *foc_driver_;
    AS5600 *as5600_;

    static void _timer_callback_static(void *args);

    void _knob_loop();

    esp_timer_handle_t knob_timer_{};

    // 棘轮吸附模式参数 (当前角度为0度，顺时针 pos 增加
    int attractor_number_ = 8;
    int attractor_current_pos_ = 0;
    // 阻尼模式参数
    float damping_gain_ = 20;
    float damping_current_pos_ = 0.0f;
    // 超出边界后反弹参数 (当前电机角度为0度来设置
    float left_boundary_rad_ = -M_PI / 2;
    float right_boundary_rad_ = M_PI / 2;

    // PID 参数
    float attractor_with_rebound_kp_ = 150.0f;
};

#endif // FOCKNOB_ROTARY_KNOB_H