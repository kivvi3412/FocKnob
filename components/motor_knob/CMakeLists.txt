file(GLOB COMPONENT_SRCS "*.cpp")

idf_component_register(SRCS ${COMPONENT_SRCS}
        INCLUDE_DIRS "include"
        REQUIRES "driver" "esp_timer" "motor_foc_driver"
)
