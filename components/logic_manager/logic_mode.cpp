//
// Created by HAIRONG ZHU on 25-2-11.
//

#include "logic_mode.h"

StartingUpMode::StartingUpMode(FocDriver *driver, PhysicalDisplay *display) {
    foc_driver_ = driver;
    physical_display_ = display;
}

StartingUpMode::~StartingUpMode() {
    this->StartingUpMode::destroy();
}

void StartingUpMode::init() {
    init_screen_ = new DisplayInit(physical_display_);
    init_screen_->init(); // 初始化开机动画
    vTaskDelay(pdMS_TO_TICKS(200));
    init_screen_->set_main_info_text("Motor Calibration...");
    foc_driver_->bsp_bridge_driver_enable(true); // 使能电机驱动
    foc_driver_->foc_motor_calibrate();
    init_screen_->set_main_info_text("Done");
    vTaskDelay(pdMS_TO_TICKS(500)); // 等待校准后电机稳定
}

void StartingUpMode::update() {
}

void StartingUpMode::stop_motor() {
    foc_driver_->set_dq(0, 0);
}

void StartingUpMode::resume_motor() {
}

void StartingUpMode::destroy() {
    foc_driver_->set_free();
    if (init_screen_) {
        init_screen_->destroy();
        delete init_screen_;
    }
}


UnboundedMode::UnboundedMode(RotaryKnob *knob, PhysicalDisplay *display) {
    rotary_knob_ = knob;
    physical_display_ = display;
}

UnboundedMode::~UnboundedMode() {
    this->UnboundedMode::destroy();
}

void UnboundedMode::init() {
    display_demo_ = new DisplayDemo(physical_display_);
    display_demo_->init();
    display_demo_->set_secondary_info_text("Unbounded Mode");
    rotary_knob_->damping(0, true);
}

void UnboundedMode::update() {
    float current_radian = rotary_knob_->get_current_radian();
    display_demo_->set_pointer_radian(current_radian);
    display_demo_->set_main_info_text(static_cast<int>(current_radian));
}

void UnboundedMode::stop_motor() {
    rotary_knob_->stop();
}

void UnboundedMode::resume_motor() {
}

void UnboundedMode::destroy() {
    rotary_knob_->stop();
    if (display_demo_) {
        display_demo_->destroy();
        delete display_demo_;
    }
}


BoundedMode::BoundedMode(RotaryKnob *knob, PhysicalDisplay *display) {
    rotary_knob_ = knob;
    physical_display_ = display;
}

BoundedMode::~BoundedMode() {
    this->BoundedMode::destroy();
}

void BoundedMode::init() {
    display_demo_ = new DisplayDemo(physical_display_);
    display_demo_->init();
    display_demo_->set_secondary_info_text("Bounded Mode 0-10\nNo Damping");
    display_demo_->create_clock_ticks_manual(-bound_range_, 15, lv_color_white());
    display_demo_->create_clock_ticks_manual(bound_range_, 15, lv_color_white());
    rotary_knob_->damping_with_rebound(0, -bound_range_, bound_range_, true);
}

void BoundedMode::update() {
    float current_radian = rotary_knob_->get_current_radian();
    if (current_radian < -bound_range_) {
        display_demo_->set_pointer_radian(-bound_range_);
        display_demo_->set_main_info_text(0);
        display_demo_->set_pressure_feedback_arc(-bound_range_, current_radian);
        display_demo_->set_background_board_percent(0);
    } else if (current_radian > bound_range_) {
        display_demo_->set_pointer_radian(bound_range_);
        display_demo_->set_main_info_text(bound_range_display_ - 1);
        display_demo_->set_pressure_feedback_arc(bound_range_, current_radian);
        display_demo_->set_background_board_percent(100);
    } else {
        float current_pointer = (current_radian + bound_range_) / (2 * bound_range_) * static_cast<float>(bound_range_display_ - 1);
        int rounded_value = static_cast<int>(std::round(current_pointer));
        display_demo_->set_pointer_radian(current_radian);
        display_demo_->set_main_info_text(rounded_value);
        display_demo_->set_pressure_feedback_arc(0, 0);
        display_demo_->set_background_board_percent(rounded_value * 100 / (bound_range_display_ - 1));
    }
}

void BoundedMode::stop_motor() {
    rotary_knob_->stop();
}

void BoundedMode::resume_motor() {
    rotary_knob_->damping_with_rebound(0, -bound_range_, bound_range_, false);
}

void BoundedMode::destroy() {
    rotary_knob_->stop();
    if (display_demo_) {
        display_demo_->destroy();
        delete display_demo_;
    }
}


SwitchMode::SwitchMode(RotaryKnob *knob, PhysicalDisplay *display) {
    rotary_knob_ = knob;
    physical_display_ = display;
}

SwitchMode::~SwitchMode() {
    this->SwitchMode::destroy();
}

void SwitchMode::init() {
    display_demo_ = new DisplayDemo(physical_display_);
    display_demo_->init();
    display_demo_->set_secondary_info_text("On/Off");
    display_demo_->create_clock_ticks_manual(-bound_range_, 15, lv_color_white());
    display_demo_->create_clock_ticks_manual(bound_range_, 15, lv_color_white());
    rotary_knob_->attractor_with_rebound(2, -bound_range_, bound_range_, true);
}

void SwitchMode::update() {
    float current_radian = rotary_knob_->get_current_radian();
    if (current_radian < -(bound_range_ + 0.05)) {
        display_demo_->set_pointer_radian(-bound_range_);
        display_demo_->set_main_info_text(0);
        display_demo_->set_pressure_feedback_arc(-bound_range_, current_radian);
        display_demo_->set_background_board_percent(0);
    } else if (current_radian > (bound_range_ + 0.05)) {
        display_demo_->set_pointer_radian(bound_range_);
        display_demo_->set_main_info_text(1);
        display_demo_->set_pressure_feedback_arc(bound_range_, current_radian);
        display_demo_->set_background_board_percent(100);
    } else {
        float current_pointer = (current_radian + bound_range_) / (2 * bound_range_) * static_cast<float>(bound_range_display_ - 1);
        int rounded_value = static_cast<int>(std::round(current_pointer));

        display_demo_->set_pointer_radian(current_radian);
        display_demo_->set_pressure_feedback_arc(0, 0);
        if (rounded_value == 0) {
            display_demo_->set_main_info_text(0);
            display_demo_->set_background_board_percent(0);
        } else {
            display_demo_->set_main_info_text(1);
            display_demo_->set_background_board_percent(100);
        }
    }
}

void SwitchMode::stop_motor() {
    rotary_knob_->stop();
}

void SwitchMode::resume_motor() {
    rotary_knob_->attractor_with_rebound(2, -bound_range_, bound_range_, false);
}

void SwitchMode::destroy() {
    rotary_knob_->stop();
    if (display_demo_) {
        display_demo_->destroy();
        delete display_demo_;
    }
}


AttractorMode::AttractorMode(RotaryKnob *knob, PhysicalDisplay *display) {
    rotary_knob_ = knob;
    physical_display_ = display;
}

AttractorMode::~AttractorMode() {
    this->AttractorMode::destroy();
}

void AttractorMode::init() {
    display_demo_ = new DisplayDemo(physical_display_);
    display_demo_->init();
    display_demo_->set_secondary_info_text("Attractor Mode");
    display_demo_->create_clock_ticks_auto(attr_number_, 15, lv_color_white());
    rotary_knob_->attractor(attr_number_, true);
}

void AttractorMode::update() {
    int attr_pos = rotary_knob_->attractor_get_pos();
    float pointer_pos = rotary_knob_->get_current_radian();

    display_demo_->set_main_info_text(attr_pos);
    display_demo_->set_pointer_radian(pointer_pos);
}


void AttractorMode::stop_motor() {
    rotary_knob_->stop();
}

void AttractorMode::resume_motor() {
    rotary_knob_->attractor(8, false);
}

void AttractorMode::destroy() {
    rotary_knob_->stop();
    if (display_demo_) {
        display_demo_->destroy();
        delete display_demo_;
    }
}
