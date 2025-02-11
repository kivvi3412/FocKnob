//
// Created by HAIRONG ZHU on 25-2-11.
//

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

class LogicManager {
public:
    LogicManager();

    void set_mode(LogicMode *mode);

private:
    LogicMode *current_mode_{};
    SemaphoreHandle_t mode_mutex_;

    static void _logic_manager_task_static(void *arg);

    void _logic_manager_main_loop();
};


#endif //FOCKNOB_LOGIC_MANAGER_H
