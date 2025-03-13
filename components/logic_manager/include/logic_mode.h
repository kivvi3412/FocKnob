//
// Created by HAIRONG ZHU on 25-2-11.
//

#ifndef FOCKNOB_LOGIC_MODE_H
#define FOCKNOB_LOGIC_MODE_H


#include "watch_dials.h"
#include "motor_knob.h"

/*
 * @brief 逻辑模式基类, 表示一种操作模式, 包括显示和电机控制的逻辑
 */
class LogicMode {
public:
    virtual ~LogicMode() = default;

    virtual void init() = 0; // 初始化模式
    virtual void destroy() = 0; // 销毁模式
    virtual void update() = 0; // 更新模式逻辑
    virtual void stop_motor() = 0; // 临时释放电机反馈，这样可以给其他东西用
    virtual void resume_motor() = 0; // 恢复旋钮对电机控制
    virtual int get_current_step_index() { return 0; }
};

/*
 * @brief 开机模式
 */
class StartingUpMode : public LogicMode {
public:
    StartingUpMode(FocDriver *driver, PhysicalDisplay *display);

    ~StartingUpMode() override;

    void init() override;

    void destroy() override;

    void update() override;

    void stop_motor() override;

    void resume_motor() override;

private:
    FocDriver *foc_driver_;
    PhysicalDisplay *physical_display_;
    DisplayInit *init_screen_{};
};

/*
 * @brief 自由转动模式
 */

class UnboundedMode : public LogicMode {
public:
    UnboundedMode(RotaryKnob *knob, PhysicalDisplay *display);

    ~UnboundedMode() override;

    void init() override;

    void destroy() override;

    void update() override;

    void stop_motor() override;

    void resume_motor() override;

private:
    float current_radian_ = 0;
    RotaryKnob *rotary_knob_;
    PhysicalDisplay *physical_display_;
    DisplayDemo *display_demo_{};
};

/*
 * @brief 有限转动模式, 无阻尼
 */
class BoundedMode : public LogicMode {
public:
    BoundedMode(RotaryKnob *knob, PhysicalDisplay *display);

    ~BoundedMode() override;

    void init() override;

    void destroy() override;

    void update() override;

    void stop_motor() override;

    void resume_motor() override;

private:
    float current_radian_ = 0;
    float bound_range_ = M_PI_4;
    int bound_range_display_ = 11;

    RotaryKnob *rotary_knob_;
    PhysicalDisplay *physical_display_;
    DisplayDemo *display_demo_{};
};

/*
 * @brief 开关模式
 */
class SwitchMode : public LogicMode {
public:
    SwitchMode(RotaryKnob *knob, PhysicalDisplay *display);

    ~SwitchMode() override;

    void init() override;

    void destroy() override;

    void update() override;

    void stop_motor() override;

    void resume_motor() override;

private:
    float current_radian_ = 0;
    float bound_range_ = M_PI / 6.0f;
    int bound_range_display_ = 2; // 2个位置

    RotaryKnob *rotary_knob_;
    PhysicalDisplay *physical_display_;
    DisplayDemo *display_demo_{};
};


/*
 * @brief 全局棘轮模式
 */
class AttractorMode : public LogicMode {
public:
    AttractorMode(RotaryKnob *knob, PhysicalDisplay *display);

    ~AttractorMode() override;

    void init() override;

    void destroy() override;

    void update() override;

    void stop_motor() override;

    void resume_motor() override;

private:
    float current_radian_ = 0;
    RotaryKnob *rotary_knob_;
    PhysicalDisplay *physical_display_;
    DisplayDemo *display_demo_{};
    int attr_number_ = 8;
};


#endif //FOCKNOB_LOGIC_MODE_H
