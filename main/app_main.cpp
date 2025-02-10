#include <esp_task_wdt.h>
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
#include "spi_display.h"
#include "watch_dials.h"

float parm_list[5] = {0, 0, 0, 0, 0};   // 设置全局参数

void foc(void *arg) {
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

RotaryKnob *r;
float range_test = M_TWOPI;
int attr_num = 17;
int damp_max = 101;

void foc_task(void *arg) {
    IICMaster iicMaster(IIC_MASTER_NUM, IIC_MASTER_SDA_IO, IIC_MASTER_SCL_IO);   // 初始化 I2C 总线
    AS5600 as5600(iicMaster.iic_master_get_bus_handle(), IIC_AS5600_ADDR);   // 初始化 AS5600 传感器
    FocDriver focDriver(FOC_MCPWM_U_GPIO, FOC_MCPWM_V_GPIO, FOC_MCPWM_W_GPIO, FOC_DRV_EN_GPIO, &as5600,
                        FOC_MOTOR_POLE_PAIRS);   // 初始化电机驱动

    vTaskDelay(pdMS_TO_TICKS(100));
    focDriver.bsp_bridge_driver_enable(true);   // 使能电机驱动
    focDriver.foc_motor_calibrate();
    vTaskDelay(pdMS_TO_TICKS(100));

    static RotaryKnob rotary_knob(&focDriver, &as5600);   // 初始化旋钮
//    rotary_knob.attractor_with_rebound(attr_num, -range_test, range_test);
    rotary_knob.damping_with_rebound(20, -range_test, range_test);
    r = &rotary_knob;


    while (true) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void lvgl_task(void *arg) {
    PhysicalDisplay physicalDisplay;
    DemoDisplay demo(&physicalDisplay);

    // 全局追踪
    demo.set_secondary_info_text("Attractor Mode");
    demo.create_clock_ticks_range(2, -range_test, range_test, 10, lv_color_white());

    vTaskDelay(pdMS_TO_TICKS(2000));
//    while (1) {
//        float current_radian = r->get_current_radian();
//        int current_attr = r->attractor_get_pos();
//
//        if (current_radian < -range_test) {
//            demo.set_main_info_text(0);
//            demo.set_radian(-range_test);
//            demo.set_pressure_feedback_arc(-range_test, current_radian);
//        } else if (current_radian > range_test) {
//            demo.set_main_info_text(attr_num - 1);
//            demo.set_radian(range_test);
//            demo.set_pressure_feedback_arc(range_test, current_radian);
//        } else {
//            demo.set_main_info_text(current_attr);
//            demo.set_radian(current_radian);
//            demo.set_pressure_feedback_arc(0, 0);
//        }
//        demo.set_background_board_percent(current_attr * 100 / (attr_num - 1));
//    }


    while (1) {
        float current_radian = r->get_current_radian();
        float current_pos = r->damping_get_pos();
        int display_pos = static_cast<int>(std::round((current_pos + range_test) * (damp_max - 1) / (2 * range_test)));

        if (current_radian < -range_test) {
            demo.set_main_info_text(0);
            demo.set_pointer_radian(-range_test);
            demo.set_pressure_feedback_arc(-range_test, current_radian);
        } else if (current_radian > range_test) {
            demo.set_main_info_text(damp_max - 1);
            demo.set_pointer_radian(range_test);
            demo.set_pressure_feedback_arc(range_test, current_radian);
        } else {
            demo.set_main_info_text(display_pos);
            demo.set_pointer_radian(current_radian);
            demo.set_pressure_feedback_arc(0, 0);
        }
        demo.set_background_board_percent(display_pos);
    }

}

void activity_monitor(void *arg) {
    // 分配足够大的缓冲区来存储任务统计信息
    char *task_stats_buffer = (char *) malloc(4096);
    if (task_stats_buffer == nullptr) {
        printf("无法分配内存来存储任务统计信息\n");
        return;
    }

    while (1) {
//         vTaskList(task_stats_buffer);   // 可以查看任务绑定在哪个CPU上了
        vTaskGetRunTimeStats(task_stats_buffer);    // 可以看CPU占用率
        printf("任务名\t\t运行时间\t\tCPU使用率 (%%) \n");
        printf("%s\n", task_stats_buffer);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main() {
    xTaskCreatePinnedToCore(foc_task, "foc_task", 16384, NULL, 5, NULL, 0);
    xTaskCreatePinnedToCore(lvgl_task, "lvgl_task", 32768, NULL, 0, NULL, 1);
    xTaskCreatePinnedToCore(activity_monitor, "activity_monitor", 4096, NULL, 1, NULL, 1);
}
