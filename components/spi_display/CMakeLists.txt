file(GLOB COMPONENT_SRCS "*.cpp")

idf_component_register(SRCS ${COMPONENT_SRCS}
        INCLUDE_DIRS "include"
        REQUIRES "driver" "lvgl" "esp_lcd_gc9a01" "esp_lvgl_port" "project_conf"
)
