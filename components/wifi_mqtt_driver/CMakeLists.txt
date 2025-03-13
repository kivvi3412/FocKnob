file(GLOB COMPONENT_SRCS *.cpp)

idf_component_register(
        SRCS ${COMPONENT_SRCS}
        INCLUDE_DIRS "include"
        REQUIRES
        nvs_flash
        esp_wifi
        esp_event
        mqtt
        json
)