//
// Created by HAIRONG ZHU on 25-2-9.
//

#include "watch_dials.h"
#include "project_conf.h"
#include <cmath>
#include <esp_log.h>

DisplayInit::DisplayInit(PhysicalDisplay *scr) {
    root_scr = scr->get_screen();
}

DisplayInit::~DisplayInit() {
    if (ui_Screen1 != nullptr) {
        lvgl_port_lock(0);
        lv_obj_del(ui_Screen1);
        lvgl_port_unlock();
    }
}

void DisplayInit::init() {
    lvgl_port_lock(0);

    ui_Screen1 = lv_obj_create(root_scr);
    lv_obj_remove_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ui_Screen1, 240, 240);  // 屏幕大小
    lv_obj_set_align(ui_Screen1, LV_ALIGN_CENTER);  // 中心对齐
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);   // 背景色
    lv_obj_set_style_border_width(ui_Screen1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);  // 边框宽度
    lv_obj_set_style_pad_all(ui_Screen1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);   // 内边距

    ui_Label1 = lv_label_create(ui_Screen1);
    lv_obj_set_width(ui_Label1, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(ui_Label1, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_align(ui_Label1, LV_ALIGN_CENTER);
    lv_label_set_text(ui_Label1, "Initializing...");
    lv_obj_set_style_text_color(ui_Label1, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(ui_Label1, 255, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_font(ui_Label1, &lv_font_montserrat_20, LV_PART_MAIN | LV_STATE_DEFAULT);

    lvgl_port_unlock();
}

void DisplayInit::destroy() {
    lvgl_port_lock(0);
    lv_obj_del(ui_Screen1);
    lvgl_port_unlock();
}

void DisplayInit::set_main_info_text(const char *text) {
    lvgl_port_lock(0);
    lv_label_set_text(ui_Label1, text);
    lvgl_port_unlock();
}


DisplayDemo::DisplayDemo(PhysicalDisplay *scr) {
    root_scr = scr->get_screen();
}

DisplayDemo::~DisplayDemo() {
    if (ui_Screen1 != nullptr) {
        lvgl_port_lock(0);
        lv_obj_del(ui_Screen1);
        lvgl_port_unlock();
    }
}

void DisplayDemo::init() {
    lvgl_port_lock(0);

    ui_Screen1 = lv_obj_create(root_scr);
    lv_obj_remove_flag(ui_Screen1, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_size(ui_Screen1, 240, 240);  // 屏幕大小
    lv_obj_set_align(ui_Screen1, LV_ALIGN_CENTER);  // 中心对齐
    lv_obj_set_style_bg_color(ui_Screen1, lv_color_black(), LV_PART_MAIN | LV_STATE_DEFAULT);   // 背景色
    lv_obj_set_style_border_width(ui_Screen1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);  // 边框宽度
    lv_obj_set_style_pad_all(ui_Screen1, 0, LV_PART_MAIN | LV_STATE_DEFAULT);   // 内边距

    background_board_ = lv_obj_create(ui_Screen1);
    lv_obj_set_size(background_board_, SPI_LCD_V_RES, SPI_LCD_H_RES); // 100x100的正方形
    lv_obj_set_align(background_board_, LV_ALIGN_TOP_MID);  // 中心对齐
    lv_obj_set_style_bg_color(background_board_, lv_color_hex(0x3D2BC0), LV_PART_MAIN | LV_STATE_DEFAULT);   // 背景色
    lv_obj_set_style_border_width(background_board_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);  // 边框宽度
    lv_obj_set_style_pad_all(background_board_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);   // 内边距
    lv_obj_set_style_radius(background_board_, 0, LV_PART_MAIN | LV_STATE_DEFAULT); // 去除圆角
    lv_obj_set_y(background_board_, SPI_LCD_H_RES); // 放到最下面，也就是显示 0%, y = 0 时显示 100%


    ui_nostylearc = lv_arc_create(ui_Screen1);  // 创建指针
    lv_obj_set_width(ui_nostylearc, 220);
    lv_obj_set_height(ui_nostylearc, 220);
    lv_obj_set_align(ui_nostylearc, LV_ALIGN_CENTER);
    lv_arc_set_range(ui_nostylearc, 0, 360);
    lv_arc_set_value(ui_nostylearc, 0);
    lv_arc_set_bg_angles(ui_nostylearc, 0, 360);
    lv_arc_set_rotation(ui_nostylearc, 270);
    lv_obj_set_style_arc_width(ui_nostylearc, 0, LV_PART_MAIN | LV_STATE_DEFAULT);

    lv_obj_set_style_arc_width(ui_nostylearc, 0, LV_PART_INDICATOR | LV_STATE_DEFAULT);

    lv_obj_set_style_bg_color(ui_nostylearc, lv_color_hex(0x5455F4), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(ui_nostylearc, 255, LV_PART_KNOB | LV_STATE_DEFAULT);

    pressure_feedback_arc_ = lv_arc_create(ui_Screen1); // 创建压力反馈弧
    lv_obj_set_width(pressure_feedback_arc_, 220);
    lv_obj_set_height(pressure_feedback_arc_, 220);
    lv_obj_set_align(pressure_feedback_arc_, LV_ALIGN_CENTER);
    lv_arc_set_range(pressure_feedback_arc_, 0, 360);
    lv_arc_set_value(pressure_feedback_arc_, 0);
    lv_arc_set_bg_angles(pressure_feedback_arc_, 0, 360);
    lv_arc_set_rotation(pressure_feedback_arc_, 270);
    lv_obj_set_style_arc_width(pressure_feedback_arc_, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_color(pressure_feedback_arc_, lv_color_hex(0x5455F4), LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_opa(pressure_feedback_arc_, 255, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_arc_width(pressure_feedback_arc_, 3, LV_PART_INDICATOR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(pressure_feedback_arc_, lv_color_hex(0x000000), LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(pressure_feedback_arc_, 0, LV_PART_KNOB | LV_STATE_DEFAULT);

    ui_textContainer1 = lv_obj_create(ui_Screen1);
    lv_obj_remove_style_all(ui_textContainer1);
    lv_obj_set_width(ui_textContainer1, 170);
    lv_obj_set_height(ui_textContainer1, 170);
    lv_obj_set_align(ui_textContainer1, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(ui_textContainer1, LV_FLEX_FLOW_COLUMN_WRAP);
    lv_obj_set_flex_align(ui_textContainer1, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(ui_textContainer1, LV_OBJ_FLAG_CLICKABLE);    /// Flags
    lv_obj_remove_flag(ui_textContainer1, LV_OBJ_FLAG_SCROLLABLE);    /// Flags

    ui_digitinfo = lv_label_create(ui_textContainer1);
    lv_obj_set_width(ui_digitinfo, LV_SIZE_CONTENT);  /// 10
    lv_obj_set_height(ui_digitinfo, LV_SIZE_CONTENT);   /// 10
    lv_obj_set_align(ui_digitinfo, LV_ALIGN_CENTER);
    lv_obj_set_flex_flow(ui_digitinfo, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(ui_digitinfo, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_START);
    lv_label_set_text(ui_digitinfo, "0");
    lv_obj_set_style_text_font(ui_digitinfo, &lv_font_montserrat_48, LV_PART_MAIN | LV_STATE_DEFAULT);

    ui_textinfo = lv_label_create(ui_textContainer1);
    lv_obj_set_width(ui_textinfo, LV_SIZE_CONTENT);  /// 1
    lv_obj_set_height(ui_textinfo, LV_SIZE_CONTENT);   /// 1
    lv_obj_set_x(ui_textinfo, 0);
    lv_obj_set_y(ui_textinfo, 30);
    lv_obj_set_align(ui_textinfo, LV_ALIGN_CENTER);
    lv_label_set_text(ui_textinfo, "Free Rotation Mode");

    lvgl_port_unlock();
}

void DisplayDemo::destroy() {
    if (ui_Screen1 != nullptr) {
        lvgl_port_lock(0);
        lv_obj_del(ui_Screen1);
        lvgl_port_unlock();
    }
}

void DisplayDemo::set_pointer_radian(float radian) {
    // radian 为弧度值，将其转换为角度值, radian 可能为负数
    int degree = (int) (radian * 180 / M_PI);
    int display_degree = _positive_fmod(degree, 360);
    if (display_degree != previous_pointer_output_) {
        lvgl_port_lock(0);
        lv_arc_set_value(ui_nostylearc, display_degree);
        lvgl_port_unlock();
        previous_pointer_output_ = display_degree;
    }
}

void DisplayDemo::set_main_info_text(int value) {
    if (value != previous_main_info_output_) {
        lvgl_port_lock(0);
        lv_label_set_text_fmt(ui_digitinfo, "%d", value);
        lvgl_port_unlock();
        previous_main_info_output_ = value;
    }
}

void DisplayDemo::set_secondary_info_text(const char *text) {
    lvgl_port_lock(0);
    lv_label_set_text(ui_textinfo, text);
    lvgl_port_unlock();
}

void DisplayDemo::create_clock_ticks_manual(float radius, float length, lv_color_t color) {
    ClockLineInfo l_info{};
    lvgl_port_lock(0);
    l_info.line = lv_line_create(ui_Screen1);
    l_info.line_points = new lv_point_precise_t[2];
    l_info.radius = radius;
    l_info.length = length;
    l_info.color = color;
    _add_clock_ticks(&l_info);
    lv_obj_move_foreground(ui_nostylearc);
    lvgl_port_unlock();
}

void DisplayDemo::create_clock_ticks_auto(int number, float length, lv_color_t color) {
    float radius = 0;
    float step = M_TWOPI / number;
    lvgl_port_lock(0);
    for (int i = 0; i < number; i++) {
        ClockLineInfo l_info{};
        l_info.line = lv_line_create(ui_Screen1);
        l_info.line_points = new lv_point_precise_t[2];
        l_info.radius = radius;
        l_info.length = length;
        l_info.color = color;
        _add_clock_ticks(&l_info);
        radius += step;
    }
    lv_obj_move_foreground(ui_nostylearc);
    lvgl_port_unlock();
}

void DisplayDemo::create_clock_ticks_range(int number, float start_rad, float end_rad, float length, lv_color_t color) {
    lvgl_port_lock(0);

    // 如果刻度数不合理，则直接返回
    if (number <= 0) {
        lvgl_port_unlock();
        return;
    }

    // 如果只有一个刻度，则只生成一个刻度，位置取 start_radius
    if (number == 1) {
        ClockLineInfo l_info{};
        l_info.line = lv_line_create(ui_Screen1);
        l_info.line_points = new lv_point_precise_t[2];
        l_info.radius = start_rad;
        l_info.length = length;
        l_info.color = color;
        _add_clock_ticks(&l_info);
    } else {
        // 计算相邻刻度之间的角度差
        float step = (end_rad - start_rad) / float(number - 1);
        for (int i = 0; i < number; i++) {
            ClockLineInfo l_info{};
            l_info.line = lv_line_create(ui_Screen1);
            l_info.line_points = new lv_point_precise_t[2];
            l_info.radius = start_rad + i * step;
            l_info.length = length;
            l_info.color = color;
            _add_clock_ticks(&l_info);
        }
    }

    lv_obj_move_foreground(ui_nostylearc);
    lvgl_port_unlock();
}

void DisplayDemo::del_clock_ticks() {
    lvgl_port_lock(0);
    _del_all_clock_ticks();
    lvgl_port_unlock();
}

void DisplayDemo::set_pressure_feedback_arc(float target_rad, float current_rad) {
    float pressure_radius = 0;
    int final_target_rad = 270 + int(target_rad * 180 / M_PI);
    if (final_target_rad != previous_pressure_feedback_arc_target_output_) {
        lvgl_port_lock(0);
        lv_arc_set_rotation(pressure_feedback_arc_, final_target_rad);
        lvgl_port_unlock();
        previous_pressure_feedback_arc_target_output_ = final_target_rad;
    }
    if (current_rad > target_rad) {
        if (previous_pressure_feedback_arc_direction_output_ != 0) {
            lvgl_port_lock(0);
            lv_arc_set_mode(pressure_feedback_arc_, LV_ARC_MODE_NORMAL);
            lvgl_port_unlock();
            previous_pressure_feedback_arc_direction_output_ = 0;
        }
        pressure_radius = _positive_fmod(current_rad - target_rad, M_TWOPI);
    } else {
        if (previous_pressure_feedback_arc_direction_output_ != 1) {
            lvgl_port_lock(0);
            lv_arc_set_mode(pressure_feedback_arc_, LV_ARC_MODE_REVERSE);
            lvgl_port_unlock();
            previous_pressure_feedback_arc_direction_output_ = 1;
        }
        pressure_radius = _positive_fmod(target_rad - current_rad, M_TWOPI);
    }
    int final_pressure_radius = int(pressure_radius * 180 / M_PI);
    if (final_pressure_radius != previous_pressure_feedback_arc_percent_output_) {
        lvgl_port_lock(0);
        lv_arc_set_value(pressure_feedback_arc_, final_pressure_radius);
        lvgl_port_unlock();
        previous_pressure_feedback_arc_percent_output_ = final_pressure_radius;
    }
}

void DisplayDemo::set_background_board_percent(int percent) {
    if (percent < 0) {
        percent = 0;
    } else if (percent > 100) {
        percent = 100;
    }
    if (percent != previous_background_board_percent_output_) {
        lvgl_port_lock(0);
        lv_obj_set_y(background_board_, SPI_LCD_H_RES - SPI_LCD_H_RES * percent / 100);
        lvgl_port_unlock();
        previous_background_board_percent_output_ = percent;
    }
}

int DisplayDemo::_positive_fmod(int x, int y) {
    int mod = std::fmod(x, y);
    return (mod < 0) ? (mod + y) : mod;
}

void DisplayDemo::_add_clock_ticks(DisplayDemo::ClockLineInfo *l_info) {    // 需要外部加锁使用
    float r = (float) SPI_LCD_H_RES / 2;

    // radius 是在某个角度上，画一条直线，直线由外向内延伸长度 length, 求出 (x1, y1) ~ (x2, y2) (仅画一根线)
    // 定义12点方向为 0 弧度，顺时针方向为正，逆时针方向为负
    l_info->line_points[0].x = lv_value_precise_t(r + r * sin(l_info->radius));
    l_info->line_points[0].y = lv_value_precise_t(r - r * cos(l_info->radius));
    l_info->line_points[1].x = lv_value_precise_t(r + (r - l_info->length) * sin(l_info->radius));
    l_info->line_points[1].y = lv_value_precise_t(r - (r - l_info->length) * cos(l_info->radius));

    // 创建线对象
    lv_line_set_points(l_info->line, l_info->line_points, 2);
    lv_obj_set_style_line_width(l_info->line, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_line_color(l_info->line, l_info->color, LV_PART_MAIN | LV_STATE_DEFAULT);
    clock_lines_.push_back(*l_info);
}

void DisplayDemo::_del_clock_ticks(DisplayDemo::ClockLineInfo *l_info) {
    lvgl_port_lock(0);
    lv_obj_del(l_info->line);
    delete[] l_info->line_points;
    lvgl_port_unlock();
}

void DisplayDemo::_del_all_clock_ticks() {
    for (auto &clock_line: clock_lines_) {
        _del_clock_ticks(&clock_line);
    }
    clock_lines_.clear();
}

float DisplayDemo::_positive_fmod(float a, float b) {
    float mod = std::fmod(a, b);
    return (mod < 0) ? (mod + b) : mod;
}
