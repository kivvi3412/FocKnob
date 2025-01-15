#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "debug_console.h"
#include "project_conf.h"
#include "iic_master.h"
#include "iic_as5600.h"
#include "motor_foc_driver.h"

#include "esp_timer.h"


float parm_list[5] = {0, 0, 0, 0, 0};   // 设置全局参数


extern "C" void app_main() {
    DebugConsole debugConsole(parm_list);   // 初始化串口调试
    IICMaster iicMaster(IIC_MASTER_NUM, IIC_MASTER_SDA_IO, IIC_MASTER_SCL_IO);   // 初始化 I2C 总线
    AS5600 as5600(iicMaster.iic_master_get_bus_handle(), IIC_AS5600_ADDR);   // 初始化 AS5600 传感器
    FocDriver focDriver(FOC_MCPWM_U_GPIO, FOC_MCPWM_V_GPIO, FOC_MCPWM_W_GPIO, FOC_DRV_EN_GPIO, &as5600,
                        FOC_MOTOR_POLE_PAIRS);   // 初始化电机驱动

    vTaskDelay(pdMS_TO_TICKS(10));
    focDriver.bsp_bridge_driver_enable(true);   // 使能电机驱动
    focDriver.foc_motor_calibrate();


    while (true) {
        if (int(parm_list[0]) == 0) {
            focDriver.set_dq_out(parm_list[2], parm_list[1]);
        } else if (int(parm_list[0]) == 1) {
            focDriver.foc_motor_calibrate();
            parm_list[0] = 0;
        } else if (int(parm_list[0]) == 2) {
            float current_angle = as5600.get_radian();
            float total_angle = as5600.get_total_radian();
            float velocity = as5600.get_velocity_filter();
            ESP_LOGI("AS5600", "Current angle: %.2f rad, Total angle: %.2f rad, Velocity: %.2f", current_angle,
                     total_angle, velocity);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }

}
