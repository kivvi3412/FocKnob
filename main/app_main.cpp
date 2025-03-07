#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "debug_console.h"
#include "logic_manager.h"
#include "logic_mode.h"
#include "pressure_sensor.h"

void activity_monitor(void *arg) {
    /*
     * @brief 任务统计信息
     *
     * 需要打开以下参数在 idf.py menuconfig 中
     * configTASKLIST_INCLUDE_COREID
     * CONFIG_FREERTOS_GENERATE_RUN_TIME_STATS          // 生成运行时间统计
     * CONFIG_FREERTOS_USE_STATS_FORMATTING_FUNCTIONS   // 使用格式化函数
     * CONFIG_FREERTOS_USE_TRACE_FACILITY               // 使用追踪功能
     */

    // 分配足够大的缓冲区来存储任务统计信息
    char *task_stats_buffer = static_cast<char *>(malloc(4096));
    if (task_stats_buffer == nullptr) {
        printf("无法分配内存来存储任务统计信息\n");
        return;
    }

    while (true) {
        // vTaskList(task_stats_buffer);   // 可以查看任务绑定在哪个CPU上了
        vTaskGetRunTimeStats(task_stats_buffer);    // 可以看CPU占用率
        printf("任务名\t\t运行时间\t\tCPU使用率 (%%) \n");
        printf("%s\n", task_stats_buffer);
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main() {
    auto *iic_master = new IICMaster(IIC_MASTER_NUM, IIC_MASTER_SDA_IO, IIC_MASTER_SCL_IO);
    auto *as5600 = new AS5600(iic_master->iic_master_get_bus_handle(), IIC_AS5600_ADDR);
    auto *foc_driver = new FocDriver(FOC_MCPWM_U_GPIO,
                                     FOC_MCPWM_V_GPIO,
                                     FOC_MCPWM_W_GPIO,
                                     FOC_DRV_EN_GPIO,
                                     as5600,
                                     FOC_MOTOR_POLE_PAIRS
    );
    auto *rotary_knob = new RotaryKnob(foc_driver, as5600);
    auto *physical_display = new PhysicalDisplay();
    auto *pressure_sensor = new PressureSensor(HX711_DOUT_GPIO, HX711_SCK_GPIO);
    auto *logic_manager = new LogicManager(pressure_sensor, foc_driver);

    // 设置开机模式
    logic_manager->set_mode(new StartingUpMode(foc_driver, physical_display));

    // 注册模式
    logic_manager->register_mode("UnboundedMode", new UnboundedMode(rotary_knob, physical_display));
    logic_manager->register_mode("BoundedMode", new BoundedMode(rotary_knob, physical_display));
    logic_manager->register_mode("SwitchMode", new SwitchMode(rotary_knob, physical_display));
    logic_manager->register_mode("AttractorMode", new AttractorMode(rotary_knob, physical_display));

    logic_manager->set_mode_by_name("UnboundedMode");

    // xTaskCreatePinnedToCore(activity_monitor, "activity_monitor", 4096, nullptr, 1, nullptr, 1);
}

