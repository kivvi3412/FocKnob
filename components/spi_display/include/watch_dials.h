//
// Created by HAIRONG ZHU on 25-2-9.
//

#ifndef FOCKNOB_WATCH_DIALS_H
#define FOCKNOB_WATCH_DIALS_H

#include <vector>
#include "spi_display.h"

/*
 * @brief 表盘都放在这里
 */
class DisplayInit : public ClockFace {  // 开机表盘
public:
    explicit DisplayInit(PhysicalDisplay *scr);

    ~DisplayInit() override;

    void init() override;

    void destroy() override;

    void set_main_info_text(const char *text);

private:
    lv_obj_t *root_scr;
    lv_obj_t *ui_Screen1{};
    lv_obj_t *ui_Label1{};
};

/*
 * @brief 功能演示用表盘
 */
class DisplayDemo : public ClockFace {
public:
    explicit DisplayDemo(PhysicalDisplay *scr);

    ~DisplayDemo() override;

    void init() override;

    void destroy() override;

    void set_pointer_radian(float radian);

    void set_main_info_text(int value);

    void set_secondary_info_text(const char *text);

    void create_clock_ticks_manual(float radius, float length, lv_color_t color); // 手动在某弧度处添加时钟刻度

    void create_clock_ticks_auto(int number, float length, lv_color_t color); // 自动添加时钟刻度(360度平分)

    void create_clock_ticks_range(int number, float start_rad, float end_rad, float length, lv_color_t color);

    void del_clock_ticks(); // 删除所有时钟刻度

    void set_pressure_feedback_arc(float target_rad, float current_rad); // 设置压力反馈弧长度

    void set_background_board_percent(int percent); // 0~100%


private:
    lv_obj_t *root_scr;
    lv_obj_t *ui_Screen1{};
    lv_obj_t *ui_nostylearc{};
    lv_obj_t *ui_textContainer1{};
    lv_obj_t *ui_digitinfo{};
    lv_obj_t *ui_textinfo{};

    struct ClockLineInfo {
        lv_obj_t *line;
        lv_point_precise_t *line_points;
        float radius;
        float length;
        lv_color_t color;
    };
    std::vector<ClockLineInfo> clock_lines_;
    lv_obj_t *pressure_feedback_arc_{};
    lv_obj_t *background_board_{};

    int previous_pointer_output_ = 0;
    int previous_main_info_output_ = 0;
    int previous_pressure_feedback_arc_target_output_ = 0;  // 压力反馈弧起始点
    int previous_pressure_feedback_arc_direction_output_ = 0;   // 压力反馈弧方向 0: 顺时针 1: 逆时针
    int previous_pressure_feedback_arc_percent_output_ = 0; // 压力反馈弧长度
    int previous_background_board_percent_output_ = 0;


    static int _positive_fmod(int x, int y);

    void _add_clock_ticks(struct ClockLineInfo *l_info);

    static void _del_clock_ticks(struct ClockLineInfo *l_info);

    void _del_all_clock_ticks();

    static float _positive_fmod(float a, float b);
};


#endif //FOCKNOB_WATCH_DIALS_H
