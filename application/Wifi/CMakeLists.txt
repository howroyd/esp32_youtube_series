set(SOURCES Wifi.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        REQUIRES esp_wifi nvs_flash esp_event esp_netif lwip)