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

    virtual void init() = 0;  // 初始化模式
    virtual void update() = 0;  // 更新模式逻辑
};

/*
 * @brief 开机模式
 */
class StartingUpMode : public LogicMode {
public:
    StartingUpMode(FocDriver *driver, PhysicalDisplay *display);

    ~StartingUpMode() override;

    void init() override;

    void update() override;

private:
    FocDriver *foc_driver_;
    PhysicalDisplay *display_;
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

    void update() override;


private:
    RotaryKnob *rotary_knob_;
    DisplayDemo *display_demo_;
};

/*
 * @brief 有限转动模式, 无阻尼
 */
class BoundedMode : public LogicMode {
public:
    BoundedMode(RotaryKnob *knob, PhysicalDisplay *display);

    ~BoundedMode() override;

    void init() override;

    void update() override;

private:
    float bound_range_ = M_PI_4;
    int bound_range_display_ = 11;

    RotaryKnob *rotary_knob_;
    DisplayDemo *display_demo_;
};

/*
 * @brief 开关模式
 */
class SwitchMode : public LogicMode {
public:
    SwitchMode(RotaryKnob *knob, PhysicalDisplay *display);

    ~SwitchMode() override;

    void init() override;

    void update() override;

private:
    float bound_range_ = M_PI / 6.0f;
    int bound_range_display_ = 2;

    RotaryKnob *rotary_knob_;
    DisplayDemo *display_demo_;
};


/*
 * @brief 全局棘轮模式
 */
class AttractorMode : public LogicMode {
public:
    AttractorMode(RotaryKnob *knob, PhysicalDisplay *display);

    ~AttractorMode() override;

    void init() override;

    void update() override;


private:
    RotaryKnob *rotary_knob_;
    DisplayDemo *display_demo_;
};


#endif //FOCKNOB_LOGIC_MODE_H
