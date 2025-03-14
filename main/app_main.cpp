#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "debug_console.h"
#include "logic_manager.h"
#include "logic_mode.h"
#include "generic_knob_mode.h"
#include "pressure_sensor.h"
#include "wifi_mqtt_driver.h"

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
        // vTaskGetRunTimeStats(task_stats_buffer); // 可以看CPU占用率
        // printf("任务名\t\t运行时间\t\tCPU使用率 (%%) \n");
        // printf("%s\n", task_stats_buffer);

        size_t free_internal = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);
        size_t free_spiram = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);
        size_t free_dma = heap_caps_get_free_size(MALLOC_CAP_DMA);
        size_t free_8bit = heap_caps_get_free_size(MALLOC_CAP_8BIT);

        ESP_LOGI("MEM_CHECK", "Free Internal RAM : %d KB", free_internal / 1024);
        ESP_LOGI("MEM_CHECK", "Free SPIRAM       : %d KB", free_spiram / 1024);
        ESP_LOGI("MEM_CHECK", "Free DMA-capable  : %d KB", free_dma / 1024);
        ESP_LOGI("MEM_CHECK", "Free 8Bit RAM     : %d KB\n", free_8bit / 1024);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main() {
    xTaskCreatePinnedToCore(activity_monitor, "activity_monitor", 4096, nullptr, 1, nullptr, 1);

    static WifiMqttDriver wifiMqtt(WIFI_SSID, WIFI_PASSWORD, MQTT_URI, MQTT_USER, MQTT_PASS, MQTT_CLIENT_ID);
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


    // 灯光开关 Light On/Off
    {
        KnobModeConfig cfg;
        cfg.displayText = "Light On/Off";
        cfg.behavior = KnobBehavior::SWITCH; // 两档开关
        cfg.minAngle = -(float) M_PI / 6.0f; // -30度
        cfg.maxAngle = (float) M_PI / 6.0f; // +30度
        cfg.steps = 1; // 两档
        cfg.start_number = 0; // 对应档位范围: [0,1]
        cfg.useRebound = true; // 开关常见使用回弹
        cfg.attractorKp = 250.0f;
        logic_manager->register_mode("LightOnOff",
                                     new GenericKnobMode(rotary_knob, physical_display, cfg));
    }

    // 灯光亮度 Light Luminance
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

    // 空调开关 AC On/Off
    {
        KnobModeConfig cfg;
        cfg.displayText = "AC On/Off";
        cfg.behavior = KnobBehavior::SWITCH; // 两档开关
        cfg.minAngle = -(float) M_PI / 6.0f; // -30度
        cfg.maxAngle = (float) M_PI / 6.0f; // +30度
        cfg.steps = 1; // 两档
        cfg.start_number = 0;
        cfg.useRebound = true;
        cfg.attractorKp = 250.0f;
        logic_manager->register_mode("ACOnOff",
                                     new GenericKnobMode(rotary_knob, physical_display, cfg));
    }

    // 空调温度 AC Temperature
    {
        KnobModeConfig cfg;
        cfg.displayText = "AC Temperature";
        cfg.behavior = KnobBehavior::ATTRACTOR;
        cfg.minAngle = -(float) M_PI * 120.0f / 180.0f; // -120度
        cfg.maxAngle = (float) M_PI * 120.0f / 180.0f; // +120度
        cfg.steps = 16; // 让 stepIndex=0..16，共17个取值
        cfg.start_number = 16; // 起始档位16 => [16..32]
        cfg.useRebound = true; // 如果想要吸引子+回弹
        cfg.attractorKp = 500.0f;
        logic_manager->register_mode("ACTemperature",
                                     new GenericKnobMode(rotary_knob, physical_display, cfg));
    }

    // 窗帘 Curtain
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
    
    wifiMqtt.connect();
    std::string previous_mqtt_json_info;
    while (true) {
        std::string current_info = logic_manager->get_mode_info_mqtt_json();
        if (current_info != previous_mqtt_json_info) {
            previous_mqtt_json_info = current_info;
            wifiMqtt.publishJson("/focknob/control", current_info, 0, false);
        }
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}
