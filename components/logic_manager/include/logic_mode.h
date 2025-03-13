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


/*
 * @brief Unity 吸顶大灯开关
 */
class UnityLightSwitchMode : public LogicMode {
public:
    UnityLightSwitchMode(RotaryKnob *knob, PhysicalDisplay *display);

    ~UnityLightSwitchMode() override;

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

    // 需要上传的数据
    bool light_switch_status_ = false; // 灯开关状态
};

/*
 * @brief Unity 吸顶大灯亮度 0~100 用阻尼模式
 */
class UnityLightLuminanceMode : public LogicMode {
public:
    UnityLightLuminanceMode(RotaryKnob *knob, PhysicalDisplay *display);

    ~UnityLightLuminanceMode() override;

    void init() override;

    void destroy() override;

    void update() override;

    void stop_motor() override;

    void resume_motor() override;

private:
    float current_radian_ = 0;
    float bound_range_ = M_PI_2;
    int bound_range_display_ = 101;
    float damping_gain_ = 15; // 阻尼增益

    RotaryKnob *rotary_knob_;
    PhysicalDisplay *physical_display_;
    DisplayDemo *display_demo_{};

    // 需要上传的数据
    int light_luminance_ = 0; // 灯亮度
};

/*
 * @brief Unity 空调开关
 */

class UnityACSwitchMode : public LogicMode {
public:
    UnityACSwitchMode(RotaryKnob *knob, PhysicalDisplay *display);

    ~UnityACSwitchMode() override;

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

    // 需要上传的数据
    bool ac_switch_status_ = false; // 空调开关状态
};

/*
 * @brief Unity 空调温度, 16 ~ 32 档位模式
 */
class UnityACTemperatureMode : public LogicMode {
public:
    UnityACTemperatureMode(RotaryKnob *knob, PhysicalDisplay *display);

    ~UnityACTemperatureMode() override;

    void init() override;

    void destroy() override;

    void update() override;

    void stop_motor() override;

    void resume_motor() override;

private:
    float current_radian_ = 0;
    float bound_range_ = M_PI * 120 / 180;
    int bound_range_display_ = 17;  // 16~32度
    float attractor_kp_ = 800;  // 吸引子力度

    RotaryKnob *rotary_knob_;
    PhysicalDisplay *physical_display_;
    DisplayDemo *display_demo_{};

    // 需要上传的数据
    int ac_temperature_ = 0; // 空调温度
};

/*
 * @brief Unity 窗帘开合大小, 阻尼模式
 */

class UnityCurtainPercentMode : public LogicMode {
public:
    UnityCurtainPercentMode(RotaryKnob *knob, PhysicalDisplay *display);

    ~UnityCurtainPercentMode() override;

    void init() override;

    void destroy() override;

    void update() override;

    void stop_motor() override;

    void resume_motor() override;

private:
    float current_radian_ = 0;
    float bound_range_ = M_PI_2;
    int bound_range_display_ = 101;
    float damping_gain_ = 15; // 阻尼增益

    RotaryKnob *rotary_knob_;
    PhysicalDisplay *physical_display_;
    DisplayDemo *display_demo_{};

    // 需要上传的数据
    int curtain_percent_ = 0; // 窗帘开合度
};

#endif //FOCKNOB_LOGIC_MODE_H
