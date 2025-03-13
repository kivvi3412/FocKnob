#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "debug_console.h"
#include "logic_manager.h"
#include "logic_mode.h"
#include "generic_knob_mode.h"
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
        vTaskGetRunTimeStats(task_stats_buffer); // 可以看CPU占用率
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
    //
    // 1) 灯光开关 Light On/Off
    //
    {
        KnobModeConfig cfg;
        cfg.displayText = "Light On/Off";
        cfg.behavior = KnobBehavior::SWITCH; // 两档开关
        cfg.minAngle = -(float) M_PI / 6.0f; // -30度
        cfg.maxAngle = (float) M_PI / 6.0f; // +30度
        cfg.steps = 1; // 两档
        cfg.start_number = 0; // 对应档位范围: [0,1]
        cfg.useRebound = true; // 开关常见使用回弹
        cfg.attractorKp = 200.0f;
        logic_manager->register_mode("LightOnOff",
                                     new GenericKnobMode(rotary_knob, physical_display, cfg));
    }

    //
    // 2) 灯光亮度 Light Luminance
    //
    {
        KnobModeConfig cfg;
        cfg.displayText = "Light Luminance";
        cfg.behavior = KnobBehavior::DAMPING_BOUND; // 阻尼 + 边界
        cfg.minAngle = -(float) M_PI_2; // -90度
        cfg.maxAngle = (float) M_PI_2; // +90度
        cfg.steps = 100; // 101 档 => 0~100
        cfg.start_number = 0; // 对应0~100
        cfg.dampingGain = 15.0f;
        logic_manager->register_mode("LightLuminance",
                                     new GenericKnobMode(rotary_knob, physical_display, cfg));
    }

    //
    // 3) 空调开关 AC On/Off
    //    与灯开关类似，仍是两档开关
    //
    {
        KnobModeConfig cfg;
        cfg.displayText = "AC On/Off";
        cfg.behavior = KnobBehavior::SWITCH; // 两档开关
        cfg.minAngle = -(float) M_PI / 6.0f; // -30度
        cfg.maxAngle = (float) M_PI / 6.0f; // +30度
        cfg.steps = 1; // 两档
        cfg.start_number = 0;
        cfg.useRebound = true;
        cfg.attractorKp = 200.0f;
        logic_manager->register_mode("ACOnOff",
                                     new GenericKnobMode(rotary_knob, physical_display, cfg));
    }

    //
    // 4) 空调温度 AC Temperature
    //    16 ~ 32度 => 一共17个值
    //    角度范围：±120度(约 ±2.094395102 rad)
    //    这时 steps=16，让离散值从 start_number(16) 到 16+16=32，共17个整数
    //    行为：ATTRACTOR (多档吸引子)
    //
    {
        KnobModeConfig cfg;
        cfg.displayText = "AC Temperature";
        cfg.behavior = KnobBehavior::ATTRACTOR;
        cfg.minAngle = -(float) M_PI * 120.0f / 180.0f; // -120度
        cfg.maxAngle = (float) M_PI * 120.0f / 180.0f; // +120度
        cfg.steps = 16; // 让 stepIndex=0..16，共17个取值
        cfg.start_number = 16; // 起始档位16 => [16..32]
        cfg.useRebound = true; // 如果想要吸引子+回弹
        cfg.attractorKp = 800.0f;
        logic_manager->register_mode("ACTemperature",
                                     new GenericKnobMode(rotary_knob, physical_display, cfg));
    }

    //
    // 5) 窗帘 Curtain
    //    0~100%，带阻尼 + 有限范围
    //    与“Light Luminance”类似
    //
    {
        KnobModeConfig cfg;
        cfg.displayText = "Curtain";
        cfg.behavior = KnobBehavior::DAMPING_BOUND;
        cfg.minAngle = -(float) M_PI_2; // -90度
        cfg.maxAngle = (float) M_PI_2; // +90度
        cfg.steps = 100; // 对应0~100
        cfg.start_number = 0;
        cfg.dampingGain = 15.0f;
        logic_manager->register_mode("Curtain",
                                     new GenericKnobMode(rotary_knob, physical_display, cfg));
    }

    logic_manager->set_mode_by_name("LightOnOff");

    // xTaskCreatePinnedToCore(activity_monitor, "activity_monitor", 4096, nullptr, 1, nullptr, 1);
}
