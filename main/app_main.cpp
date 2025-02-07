#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "debug_console.h"
#include "project_conf.h"
#include "iic_master.h"
#include "iic_as5600.h"
#include "motor_foc_driver.h"
#include "motor_pid_controller.h"
#include "motor_knob.h"

float parm_list[5] = {0, 0, 0, 0, 0};   // 设置全局参数


extern "C" void app_main() {
    DebugConsole debugConsole(parm_list);   // 初始化串口调试
    IICMaster iicMaster(IIC_MASTER_NUM, IIC_MASTER_SDA_IO, IIC_MASTER_SCL_IO);   // 初始化 I2C 总线
    AS5600 as5600(iicMaster.iic_master_get_bus_handle(), IIC_AS5600_ADDR);   // 初始化 AS5600 传感器
    FocDriver focDriver(FOC_MCPWM_U_GPIO, FOC_MCPWM_V_GPIO, FOC_MCPWM_W_GPIO, FOC_DRV_EN_GPIO, &as5600,
                        FOC_MOTOR_POLE_PAIRS);   // 初始化电机驱动
    PIDController pid_velocity(10, 300, 0, FOC_MCPWM_OUTPUT_LIMIT, FOC_MCPWM_OUTPUT_LIMIT, 0);   // 初始化速度 PID 控制器
    PIDController pid_position(10, 0, 0, FOC_MCPWM_OUTPUT_LIMIT, FOC_MCPWM_OUTPUT_LIMIT, 0);
    PIDController pid_position_velocity(10, 30, 0, FOC_MCPWM_OUTPUT_LIMIT, 3, 0);
    RotaryKnob rotary_knob(&focDriver, &as5600);   // 初始化旋钮

    vTaskDelay(pdMS_TO_TICKS(10));
    focDriver.bsp_bridge_driver_enable(true);   // 使能电机驱动
    focDriver.foc_motor_calibrate();


    while (true) {
        if (int(parm_list[0]) == 0) {
            // 什么都不做
        } else if (int(parm_list[0]) == 1) {
            rotary_knob.attractor(int(parm_list[1]));
            parm_list[0] = 0;
        } else if (int(parm_list[0]) == 2) {
            rotary_knob.attractor_with_rebound(int(parm_list[1]), float(parm_list[2]), float(parm_list[3]));
            parm_list[0] = 0;
        } else if (int(parm_list[0]) == 3) {
            rotary_knob.damping(float(parm_list[1]));
            parm_list[0] = 0;
        } else if (int(parm_list[0]) == 4) {
            rotary_knob.damping_with_rebound(float(parm_list[1]), float(parm_list[2]), float(parm_list[3]));
            parm_list[0] = 0;
        } else if (int(parm_list[0]) == 5) {
            ESP_LOGI("app_main", "damping: %f", rotary_knob.damping_get_pos());
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

}
