#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "esp_err.h"

extern "C" void app_main() {
    while (1) {
        ESP_LOGI("main", "Hello World!");
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}