#ifndef GENERIC_KNOB_MODE_H
#define GENERIC_KNOB_MODE_H

#include <string>
#include <cmath>
#include <algorithm> // std::round
#include "logic_mode.h"
#include "motor_knob.h"
#include "watch_dials.h"

/**
 * @brief 不同旋钮交互类型的枚举
 */
enum class KnobBehavior {
    UNBOUNDED, ///< 无限旋转
    BOUNDED, ///< 有限范围，无阻尼
    SWITCH, ///< 例如 2 档开关
    ATTRACTOR, ///< 多档吸引子
    DAMPING_BOUND ///< 阻尼 + 有限范围
};

/**
 * @brief 通用旋钮模式的配置结构
 */
struct KnobModeConfig {
    std::string displayText; ///< UI上显示的标题/说明

    KnobBehavior behavior; ///< 当前模式交互类型
    float minAngle; ///< 角度下限(用于有限范围)
    float maxAngle; ///< 角度上限(用于有限范围)

    int steps = 0; ///< 档位数(大于1才生效，供吸引子/多档模式)
    int start_number = 0; ///< 起始档位(适用于吸引子或多档模式)
    ///< 最小值是 start_number, 最大值是 start_number+steps

    bool useRebound = false; ///< 是否带回弹(对应 damping_with_rebound 或 attractor_with_rebound)
    float dampingGain = 0.0f; ///< 阻尼增益(用于阻尼模式)
    float attractorKp = 800.0f; ///< 吸引子力度(用于吸引子模式)
};

/**
 * @brief 通用的“旋钮逻辑模式”组件，根据 KnobModeConfig 参数实现不同功能
 */
class GenericKnobMode : public LogicMode {
public:
    /**
     * @brief 构造函数
     * @param knob 旋钮控制(电机+编码器)
     * @param display 实际显示屏操作对象
     * @param config 模式配置
     */
    GenericKnobMode(RotaryKnob *knob,
                    PhysicalDisplay *display,
                    const KnobModeConfig &config);

    ~GenericKnobMode() override;

    void init() override; ///< 初始化模式
    void update() override; ///< 周期更新
    void destroy() override; ///< 释放资源

    void stop_motor() override; ///< 临时停止电机
    void resume_motor() override; ///< 恢复电机

    int get_current_step_index() override;   //// 返回模式名称和索引位置

private:
    RotaryKnob *knob_ = nullptr;
    PhysicalDisplay *display_ = nullptr;
    DisplayDemo *displayDemo_ = nullptr;

    KnobModeConfig config_; ///< 模式配置
    float currentAngle_ = 0.0f;
    bool reset_custom_angle_ = true;
    int current_step_index_ = 0; ///< 当前档位索引
};

#endif // GENERIC_KNOB_MODE_H
