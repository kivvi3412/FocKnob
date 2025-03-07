// logic_manager.h
#ifndef FOCKNOB_LOGIC_MANAGER_H
#define FOCKNOB_LOGIC_MANAGER_H

#include "project_conf.h"
#include "iic_master.h"
#include "iic_as5600.h"
#include "motor_foc_driver.h"
#include "motor_pid_controller.h"
#include "motor_knob.h"
#include "spi_display.h"
#include "watch_dials.h"
#include "logic_mode.h"
#include "pressure_sensor.h"
#include <vector>
#include <string>
#include <algorithm>

/*
 * @brief 逻辑管理器, 负责管理逻辑模式, 电机驱动, 按钮力反馈
 */

class LogicManager {
public:
    LogicManager(PressureSensor *pressure_sensor, FocDriver *foc_driver);

    void set_mode(LogicMode *mode);  // 保留原有的设置模式方法
    void register_mode(const std::string& name, LogicMode* mode);  // 注册模式
    void set_mode_by_name(const std::string& name);  // 通过名称设置模式
    void set_next_mode();  // 切换到下一个模式

private:
    PressureSensor *pressure_sensor_;    // 压力传感器 (用于按钮力反馈)
    FocDriver *foc_driver_;              // 电机驱动

    LogicMode *current_mode_{};
    SemaphoreHandle_t mode_mutex_;

    std::vector<std::pair<std::string, LogicMode*>> modes_;  // 存储模式名称和对应的模式对象
    size_t current_mode_index_ = 0;  // 当前模式的索引

    bool previous_pressed_ = false;

    void _on_press() const;

    void _on_release();

    static void _logic_manager_task_static(void *arg);

    void _logic_manager_main_loop();
};

#endif //FOCKNOB_LOGIC_MANAGER_H