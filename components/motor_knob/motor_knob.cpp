#include "motor_knob.h"
#include "project_conf.h" // 包含项目配置, 例如 FOC_CALC_PERIOD

#include <cmath>

#define _constrain(amt, low, high) ((amt)<(low)?(low):((amt)>(high)?(high):(amt)))

RotaryKnob::RotaryKnob(FocDriver *focDriver, AS5600 *as5600)
    : foc_driver_(focDriver), as5600_(as5600) {
    const esp_timer_create_args_t timer_args = {
        .callback = &RotaryKnob::_timer_callback_static,
        .arg = this,
        .name = "rotary_knob_timer"
    };
    ESP_ERROR_CHECK(esp_timer_create(&timer_args, &knob_timer_));
    ESP_ERROR_CHECK(esp_timer_start_periodic(knob_timer_, FOC_CALC_PERIOD));
}

void RotaryKnob::stop() {
    current_mode_ = Mode::None;
    foc_driver_->set_dq(0, 0);
}

// 如果不重置，使用current_radian
void RotaryKnob::attractor(int attractor_num, bool reset_custom_pos, float current_radian) {
    attractor_number_ = (attractor_num < 1) ? 1 : attractor_num;
    if (reset_custom_pos) {
        as5600_->reset_custom_total_radian(); // 重置自定义总弧度
    } else {
        as5600_->set_custom_total_radian(current_radian);
    }
    current_mode_ = Mode::Attractor;
}

/*
 * @brief 设置棘轮吸附模式，超出边界后反弹, 当前角度为0度，指向left_rad, 顺时针旋转pos增大，如果 a_num = 2 则有吸附点 0 和 1
 */
void RotaryKnob::attractor_with_rebound(int attractor_num, float left_rad, float right_rad, bool reset_custom_pos,
                                        float current_radian) {
    attractor_number_ = (attractor_num < 1) ? 1 : attractor_num - 1;
    left_boundary_rad_ = left_rad;
    right_boundary_rad_ = right_rad;

    if (reset_custom_pos) {
        as5600_->set_custom_total_radian(left_rad); // 重置自定义总弧度
    } else {
        as5600_->set_custom_total_radian(current_radian);
    }
    current_mode_ = Mode::AttractorWithRebound;
}

void RotaryKnob::damping(float damping_gain, bool reset_custom_pos, float current_radian) {
    damping_gain_ = damping_gain;

    if (reset_custom_pos) {
        as5600_->reset_custom_total_radian(); // 重置自定义总弧度
    } else {
        as5600_->set_custom_total_radian(current_radian);
    }
    current_mode_ = Mode::Damping;
}

void RotaryKnob::damping_with_rebound(float damping_gain, float left_rad, float right_rad, bool reset_custom_pos,
                                      float current_radian) {
    damping_gain_ = damping_gain;
    left_boundary_rad_ = left_rad;
    right_boundary_rad_ = right_rad;

    if (reset_custom_pos) {
        as5600_->set_custom_total_radian(left_rad); // 重置自定义总弧度
    } else {
        as5600_->set_custom_total_radian(current_radian);
    }
    current_mode_ = Mode::DampingWithRebound;
}

int RotaryKnob::attractor_get_pos() const {
    return attractor_current_pos_;
}

float RotaryKnob::damping_get_pos() const {
    return damping_current_pos_;
}

float RotaryKnob::get_current_radian() const {
    return as5600_->get_custom_total_radian();
}


// private
void RotaryKnob::_timer_callback_static(void *args) {
    auto *self = static_cast<RotaryKnob *>(args);
    self->_knob_loop();
}

void RotaryKnob::_knob_loop() {
    float current_rad = as5600_->get_custom_total_radian();

    switch (current_mode_) {
        case Mode::Attractor: {
            // 棘轮吸附模式
            // 计算一个简单的kp, 并做饱和处理
            float kp = 100.0f * logf((float) attractor_number_ + 1.0f) + 100.0f;
            if (kp > 1000.0f) kp = 1000.0f; // 饱和上限

            float attractor_distance = float(M_TWOPI) / float(attractor_number_);
            // 找到最近的吸附点(可简单用 round())
            float target = std::round(current_rad / attractor_distance) * attractor_distance;
            // 更新当前吸附点
            attractor_current_pos_ = int(target / attractor_distance);
            float error = target - current_rad;

            // 设置力矩： dq_out(0, kp*error)
            float Uq = _constrain(kp * error, -FOC_MCPWM_OUTPUT_LIMIT / 3.0f, FOC_MCPWM_OUTPUT_LIMIT / 3.0f);
            foc_driver_->set_dq(0, Uq);
            break;
        }
        case Mode::AttractorWithRebound: {
            // 棘轮吸附模式，超出边界后反弹
            // 计算吸附位置
            float range_rad = right_boundary_rad_ - left_boundary_rad_;
            float attractor_distance = range_rad / float(attractor_number_);
            int attractor_index = static_cast<int>(roundf((current_rad - left_boundary_rad_) / attractor_distance));
            // 限制吸附点索引
            attractor_index = _constrain(attractor_index, 0, attractor_number_);
            // 计算目标位置
            float target_rad = left_boundary_rad_ + float(attractor_index) * attractor_distance;
            // 更新当前吸附位置
            attractor_current_pos_ = attractor_index;
            // 计算误差和控制力矩
            float error = target_rad - current_rad;
            float kp = 150.0f;
            float torque = _constrain(kp * error, -FOC_MCPWM_OUTPUT_LIMIT / 3.0f, FOC_MCPWM_OUTPUT_LIMIT / 3.0f);
            foc_driver_->set_dq(0, torque);
            break;
        }
        case Mode::Damping: {
            damping_current_pos_ = current_rad;
            float velocity = as5600_->get_velocity_filter();
            if (std::fabs(velocity) < 0.1f) {
                velocity = 0.0f;
            }
            float torque = -damping_gain_ * velocity;
            torque = _constrain(torque, -FOC_MCPWM_OUTPUT_LIMIT / 3.0f, FOC_MCPWM_OUTPUT_LIMIT / 3.0f);
            foc_driver_->set_dq(0, torque);
            break;
        }
        case Mode::DampingWithRebound: {
            if (current_rad < left_boundary_rad_ || current_rad > right_boundary_rad_) {
                float error = 0.0f;
                if (current_rad < left_boundary_rad_) {
                    error = left_boundary_rad_ - current_rad;
                } else if (current_rad > right_boundary_rad_) {
                    error = right_boundary_rad_ - current_rad;
                }
                float kp = 150.0f; // 自行调参
                float torque = _constrain(kp * error, -FOC_MCPWM_OUTPUT_LIMIT / 3.0f, FOC_MCPWM_OUTPUT_LIMIT / 3.0f);
                foc_driver_->set_dq(0, torque);
            } else {
                damping_current_pos_ = current_rad;
                float velocity = as5600_->get_velocity_filter();
                if (std::fabs(velocity) < 0.1f) {
                    velocity = 0.0f;
                }
                float torque = -damping_gain_ * velocity;
                torque = _constrain(torque, -FOC_MCPWM_OUTPUT_LIMIT / 3.0f, FOC_MCPWM_OUTPUT_LIMIT / 3.0f);
                foc_driver_->set_dq(0, torque);
            }
            break;
        }
        case Mode::None: {
            break;
        }
    }
}
