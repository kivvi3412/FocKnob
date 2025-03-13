//
// Created by HAIRONG ZHU on 25-2-8.
//

#include "spi_display.h"
#include "project_conf.h"
#include "esp_log.h"
#include "driver/gpio.h"

auto TAG = "DISPLAY";

PhysicalDisplay::PhysicalDisplay(lv_display_rotation_t screen_rotation) {
    ESP_LOGI(TAG, "Enable LCD Background Light");
    gpio_config_t drv_en_config = {
        .pin_bit_mask = 1ULL << SPI_LCD_EN,
        .mode = GPIO_MODE_OUTPUT,
    };
    ESP_ERROR_CHECK(gpio_config(&drv_en_config));
    ESP_ERROR_CHECK(gpio_set_level(SPI_LCD_EN, 1)); // 使能背光灯

    ESP_LOGI(TAG, "Initialize SPI bus");
    constexpr spi_bus_config_t bus_config = GC9A01_PANEL_BUS_SPI_CONFIG(SPI_LCD_PCLK, SPI_LCD_MOSI,
                                                                        240 * 10 * sizeof(uint16_t)
    );
    ESP_ERROR_CHECK(spi_bus_initialize(SPI_LCD_HOST, &bus_config, SPI_DMA_CH_AUTO));

    ESP_LOGI(TAG, "Install panel IO");
    esp_lcd_panel_io_handle_t io_handle = nullptr;
    constexpr esp_lcd_panel_io_spi_config_t io_config = GC9A01_PANEL_IO_SPI_CONFIG(SPI_LCD_CS,
        SPI_LCD_DC,
        nullptr, nullptr);
    ESP_ERROR_CHECK(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t) SPI_LCD_HOST, &io_config, &io_handle));

    ESP_LOGI(TAG, "Install GC9A01 panel driver");
    esp_lcd_panel_handle_t panel_handle = nullptr;

    constexpr esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = SPI_LCD_RST,
        .rgb_ele_order = LCD_RGB_ELEMENT_ORDER_BGR,
        .bits_per_pixel = 16,
    };
    ESP_ERROR_CHECK(esp_lcd_new_panel_gc9a01(io_handle, &panel_config, &panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_reset(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_init(panel_handle));
    ESP_ERROR_CHECK(esp_lcd_panel_invert_color(panel_handle, true));
    ESP_ERROR_CHECK(esp_lcd_panel_disp_on_off(panel_handle, true));
    // 以上代码初始化了 SPI 总线、LCD IO 和 LCD 驱动，使 LCD 显示器处于开机状态

    constexpr lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 6144,
        .task_affinity = 0,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5,
    };;
    ESP_ERROR_CHECK(lvgl_port_init(&lvgl_cfg));

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = io_handle,
        .panel_handle = panel_handle,
        .buffer_size = SPI_LCD_H_RES * SPI_LCD_V_RES, // 修改为实际的显示器垂直分辨率
        .double_buffer = true,
        .hres = SPI_LCD_H_RES,
        .vres = SPI_LCD_V_RES,
        .monochrome = false,
        .rotation = {
            .swap_xy = false,
            .mirror_x = true,
            .mirror_y = false,
        },
        .color_format = LV_COLOR_FORMAT_RGB565,
        .flags = {
            .buff_dma = true,
            .buff_spiram = true,
            .swap_bytes = true,
        }
    };
    // 使用 display_handle 句柄，可以通过该句柄操作 LVGL
    static lv_disp_t *display_handle = lvgl_port_add_disp(&disp_cfg);

    /*
     * 1. `lv_timer_handler` 要定期调用，所以不要在这个循环里写业务代码。
     * 2. `lv_timer_handler` 调用期间，不要有另外一个线程更改 LVGL 的元素（比如 `lv_text` 的文本，各种元素的颜色，控件的位置）。
     *    - 怎么判断是不是 “另外一个线程”：
     *    - LVGL 事件回调都不是 “另外一个线程”，可以安全更改 UI。
     *    - 你自己创建的线程都是 “另外一个线程”，要确保 `lv_timer_handler` 执行期间不要更改 LVGL 的任何内容。
     * 3. 避免手动调用 lv_timer_handler(): 如果使用的是 esp_lvgl_port, 它已经在内部为您处理了 LVGL 的定时器和刷新任务。
     *    - 不需要也不应该在自己的任务中再次调用 lv_timer_handler(), 这可能会导致不一致
     */
    // 设置暗黑主题
    lvgl_port_lock(0); // 如果不加锁，可能会导致 LVGL 崩溃
    lv_theme_default_init(display_handle,
                          lv_palette_main(LV_PALETTE_BLUE),
                          lv_palette_main(LV_PALETTE_RED),
                          true,
                          LV_FONT_DEFAULT);
    // 旋转屏幕角度(逆时针方向)
    lv_disp_set_rotation(display_handle, screen_rotation);

    lvgl_port_unlock();
    // 创建抽象物理屏幕
    screen_ = lv_disp_get_scr_act(display_handle);
}

lv_obj_t *PhysicalDisplay::get_screen() const {
    return screen_;
}
