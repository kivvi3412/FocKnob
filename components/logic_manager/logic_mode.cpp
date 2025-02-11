//
// Created by HAIRONG ZHU on 25-2-11.
//

#include "logic_mode.h"

StartingUpMode::StartingUpMode(FocDriver *driver, PhysicalDisplay *display) {
    foc_driver_ = driver;
    display_ = display;
}

void StartingUpMode::init() {
    init_screen_ = new DisplayInit(display_);
    init_screen_->init(); // 初始化开机动画
    vTaskDelay(pdMS_TO_TICKS(200));
    init_screen_->set_main_info_text("Motor Calibration...");
    foc_driver_->bsp_bridge_driver_enable(true);   // 使能电机驱动
    foc_driver_->foc_motor_calibrate();
    init_screen_->set_main_info_text("Done");
    vTaskDelay(pdMS_TO_TICKS(500)); // 等待校准后电机稳定
}

void StartingUpMode::update() {}

StartingUpMode::~StartingUpMode() {
    init_screen_->destroy();
}


UnboundedMode::UnboundedMode(RotaryKnob *knob, PhysicalDisplay *display) {
    rotary_knob_ = knob;
    display_demo_ = new DisplayDemo(display);
}

UnboundedMode::~UnboundedMode() {
    display_demo_->destroy();
}

void UnboundedMode::init() {
    display_demo_->init();
    display_demo_->set_secondary_info_text("Unbounded Mode");
    rotary_knob_->damping(0);
}

void UnboundedMode::update() {
    float current_radian = rotary_knob_->get_current_radian();
    display_demo_->set_pointer_radian(current_radian);
    display_demo_->set_main_info_text(int(current_radian));
}


BoundedMode::BoundedMode(RotaryKnob *knob, PhysicalDisplay *display) {
    rotary_knob_ = knob;
    display_demo_ = new DisplayDemo(display);
}

BoundedMode::~BoundedMode() {
    display_demo_->destroy();
}

void BoundedMode::init() {
    display_demo_->init();
    display_demo_->set_secondary_info_text("Bounded Mode 0-10\nNo Damping");
    display_demo_->create_clock_ticks_manual(-bound_range_, 15, lv_color_white());
    display_demo_->create_clock_ticks_manual(bound_range_, 15, lv_color_white());
    rotary_knob_->damping_with_rebound(0, -bound_range_, bound_range_);
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
        float current_pointer = (current_radian + bound_range_) / (2 * bound_range_) * float(bound_range_display_ - 1);
        int rounded_value = static_cast<int>(std::round(current_pointer));
        display_demo_->set_pointer_radian(current_radian);
        display_demo_->set_main_info_text(rounded_value);
        display_demo_->set_pressure_feedback_arc(0, 0);
        display_demo_->set_background_board_percent(rounded_value * 100 / (bound_range_display_ - 1));
    }
}


SwitchMode::SwitchMode(RotaryKnob *knob, PhysicalDisplay *display) {
    rotary_knob_ = knob;
    display_demo_ = new DisplayDemo(display);
}

SwitchMode::~SwitchMode() {
    display_demo_->destroy();
}

void SwitchMode::init() {
    display_demo_->init();
    display_demo_->set_secondary_info_text("On/Off");
    display_demo_->create_clock_ticks_manual(-bound_range_, 15, lv_color_white());
    display_demo_->create_clock_ticks_manual(bound_range_, 15, lv_color_white());
    rotary_knob_->attractor_with_rebound(2, -bound_range_, bound_range_);
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
        float current_pointer = (current_radian + bound_range_) / (2 * bound_range_) * float(bound_range_display_ - 1);
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


AttractorMode::AttractorMode(RotaryKnob *knob, PhysicalDisplay *display) {
    rotary_knob_ = knob;
    display_demo_ = new DisplayDemo(display);
}

void AttractorMode::init() {
    int attr_number = 8;

    display_demo_->init();
    display_demo_->set_secondary_info_text("Attractor Mode");
    display_demo_->create_clock_ticks_auto(attr_number, 15, lv_color_white());
    rotary_knob_->attractor(attr_number);
}

void AttractorMode::update() {
    int attr_pos = rotary_knob_->attractor_get_pos();
    float pointer_pos = rotary_knob_->get_current_radian();

    display_demo_->set_main_info_text(attr_pos);
    display_demo_->set_pointer_radian(pointer_pos);
}


AttractorMode::~AttractorMode() {
    display_demo_->destroy();
}
