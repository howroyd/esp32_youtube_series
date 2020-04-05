set(SOURCES ./Src/main.cpp ./Src/heap.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS . Inc ../esp32_include_path_fix
                        REQUIRES freertos esp_common
                        PRIV_REQUIRES heap esp_event nvs_flash Gpio SntpTime MqttClient)