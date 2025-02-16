file(GLOB COMPONENT_SRCS "*.cpp")

idf_component_register(SRCS ${COMPONENT_SRCS}
        INCLUDE_DIRS "include"
        REQUIRES "driver" "iic_master" "iic_as5600" "motor_foc_driver" "motor_pid_controller" "motor_knob" "spi_display" "pressure_sensor"
)
