set(SOURCES Wifi.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        PRIV_INCLUDE_DIRS 
                        REQUIRES core esp_wifi esp_event nvs_flash esp_netif
                        PRIV_REQUIRES lwip )