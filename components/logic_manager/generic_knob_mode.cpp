#include "generic_knob_mode.h"

GenericKnobMode::GenericKnobMode(RotaryKnob *knob,
                                 PhysicalDisplay *display,
                                 const KnobModeConfig &config)
    : knob_(knob),
      display_(display),
      config_(config),
      currentAngle_(0.0f) {
}

GenericKnobMode::~GenericKnobMode() {
    GenericKnobMode::destroy();
}

void GenericKnobMode::init() {
    // 1) 初始化显示界面
    displayDemo_ = new DisplayDemo(display_);
    displayDemo_->init();
    displayDemo_->set_secondary_info_text(config_.displayText.c_str());

    // 2) 如果是有限范围(BOUNDED / DAMPING_BOUND / 等)，可在最小角度和最大角度绘制刻度
    //    以下只是演示，可自行根据需求微调
    if (config_.behavior == KnobBehavior::BOUNDED ||
        config_.behavior == KnobBehavior::DAMPING_BOUND) {
        displayDemo_->create_clock_ticks_manual(config_.minAngle, 15, lv_color_white());
        displayDemo_->create_clock_ticks_manual(config_.maxAngle, 15, lv_color_white());
    } else if (config_.behavior == KnobBehavior::ATTRACTOR) {
        displayDemo_->create_clock_ticks_range(config_.steps, config_.minAngle, config_.maxAngle, 15, lv_color_white());
    } else if (config_.behavior == KnobBehavior::SWITCH) {
        displayDemo_->create_clock_ticks_manual(config_.minAngle, 15, lv_color_white());
        displayDemo_->create_clock_ticks_manual(config_.maxAngle, 15, lv_color_white());
    }

    // 3) 根据不同的 behavior 设置旋钮的电机控制模式
    switch (config_.behavior) {
        case KnobBehavior::UNBOUNDED:
            // 无限旋转，无阻尼 or 0阻尼
            knob_->damping(0.0f, reset_custom_angle_, currentAngle_);
            break;

        case KnobBehavior::BOUNDED:
            // 有边界，无阻尼
            knob_->damping_with_rebound(
                0.0f, config_.minAngle, config_.maxAngle,
                reset_custom_angle_, currentAngle_);
            break;

        case KnobBehavior::SWITCH:
            // 假设 SWITCH 就是 2 档吸引子 + 回弹
            knob_->attractor_with_rebound(
                config_.steps + 1,
                config_.minAngle, config_.maxAngle,
                reset_custom_angle_, currentAngle_,
                config_.attractorKp);
            break;

        case KnobBehavior::ATTRACTOR:
            // 多档吸引子
            if (config_.useRebound) {
                knob_->attractor_with_rebound(
                    config_.steps,
                    config_.minAngle, config_.maxAngle,
                    reset_custom_angle_, currentAngle_,
                    config_.attractorKp);
            } else {
                knob_->attractor(config_.steps, true, currentAngle_);
            }
            break;

        case KnobBehavior::DAMPING_BOUND:
            // 阻尼 + 边界回弹
            knob_->damping_with_rebound(
                config_.dampingGain,
                config_.minAngle, config_.maxAngle,
                reset_custom_angle_, currentAngle_);
            break;
    }

    // 4) 设置初始指针
    displayDemo_->set_pointer_radian(currentAngle_);
    displayDemo_->show_pointer(true);
    reset_custom_angle_ = false;
}

void GenericKnobMode::update() {
    // 获取当前角度
    currentAngle_ = knob_->get_current_radian();

    // 辅助变量：角度范围
    float range = config_.maxAngle - config_.minAngle;

    // 1) 如果超出下限
    if (currentAngle_ < config_.minAngle) {
        displayDemo_->set_pointer_radian(config_.minAngle);
        displayDemo_->set_main_info_text(config_.start_number);
        displayDemo_->set_pressure_feedback_arc(config_.minAngle, currentAngle_);
        displayDemo_->set_background_board_percent(0);
        return;
    }

    // 2) 如果超出上限
    if (currentAngle_ > config_.maxAngle) {
        displayDemo_->set_pointer_radian(config_.maxAngle);
        displayDemo_->set_main_info_text(config_.start_number + config_.steps);
        displayDemo_->set_pressure_feedback_arc(config_.maxAngle, currentAngle_);
        displayDemo_->set_background_board_percent(100);
        return;
    }

    // 3) 在正常范围内
    float progress = 0.0f;
    if (std::fabs(range) > 1e-6) {
        progress = (currentAngle_ - config_.minAngle) / range; // 0 ~ 1
    }

    // 如果 steps > 1，则表示多档模式，需要计算当前档位
    // 档位下限是 start_number，档位上限是 start_number + steps
    if (config_.steps >= 1) {
        float stepFloat = progress * (float) (config_.steps);
        // 四舍五入到最近档位
        int stepIndex = static_cast<int>(std::round(stepFloat));

        // 计算实际数值(加上起始档位)
        int realValue = config_.start_number + stepIndex;

        // 显示
        displayDemo_->set_main_info_text(realValue);
        current_step_index_ = realValue;

        // 进度条（相对 steps）
        float stepProgress = (float) stepIndex / (float) (config_.steps);
        int percent = static_cast<int>(std::round(stepProgress * 100.0f));
        displayDemo_->set_background_board_percent(percent);
    } else {
        // 否则就显示百分比
        int percent = static_cast<int>(std::round(progress * 100.0f));
        displayDemo_->set_main_info_text(percent);
        displayDemo_->set_background_board_percent(percent);
        current_step_index_ = percent;
    }

    // 指针位置、压感弧
    displayDemo_->set_pointer_radian(currentAngle_);
    displayDemo_->set_pressure_feedback_arc(0, 0);
}

void GenericKnobMode::destroy() {
    stop_motor();
    if (displayDemo_) {
        displayDemo_->destroy();
        delete displayDemo_;
        displayDemo_ = nullptr;
    }
}

void GenericKnobMode::stop_motor() {
    if (knob_) {
        knob_->stop();
    }
}

void GenericKnobMode::resume_motor() {
    // 根据不同的 behavior 做相应的恢复
    // 注意：这里不会重新 new DisplayDemo，也不会清理 UI
    //       只恢复旋钮的电机模式即可
    switch (config_.behavior) {
        case KnobBehavior::UNBOUNDED:
            knob_->damping(0.0f, false, currentAngle_);
            break;

        case KnobBehavior::BOUNDED:
            knob_->damping_with_rebound(
                0.0f,
                config_.minAngle, config_.maxAngle,
                false,
                currentAngle_);
            break;

        case KnobBehavior::SWITCH:
            knob_->attractor_with_rebound(
                config_.steps,
                config_.minAngle, config_.maxAngle,
                false,
                currentAngle_,
                config_.attractorKp);
            break;

        case KnobBehavior::ATTRACTOR:
            if (config_.useRebound) {
                knob_->attractor_with_rebound(
                    config_.steps,
                    config_.minAngle, config_.maxAngle,
                    false, currentAngle_,
                    config_.attractorKp);
            } else {
                knob_->attractor(config_.steps, false, currentAngle_);
            }
            break;

        case KnobBehavior::DAMPING_BOUND:
            knob_->damping_with_rebound(
                config_.dampingGain,
                config_.minAngle, config_.maxAngle,
                false,
                currentAngle_);
            break;
    }

    // 如有需要，可在此设置一次指针(可选)
    // displayDemo_->set_pointer_radian(currentAngle_);
}

int GenericKnobMode::get_current_step_index() {
    return current_step_index_;
}