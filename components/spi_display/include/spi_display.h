//
// Created by HAIRONG ZHU on 25-2-8.
//

#ifndef FOCKNOB_SPI_DISPLAY_H
#define FOCKNOB_SPI_DISPLAY_H

#include "esp_lcd_gc9a01.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_ops.h"
#include "lvgl.h"
#include "esp_lvgl_port.h"
#include "esp_lvgl_port_disp.h"

/*
 * @brief 物理显示器放在这, 如果有多块显示屏
 */


// 物理显示器类，任何 watch dial 都需要同一个物理显示器
class PhysicalDisplay {
public:
    PhysicalDisplay();

    lv_obj_t *get_screen(); // 获取屏幕对象

private:
    lv_obj_t *screen_;
};


class ClockFace {
public:
    virtual ~ClockFace() = default; // 纯虚函数, 派生类需要实现

    virtual void init() = 0;

    virtual void destroy() = 0;
};

#endif //FOCKNOB_SPI_DISPLAY_H
