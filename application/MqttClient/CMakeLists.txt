set(SOURCES MqttClient.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        PRIV_INCLUDE_DIRS include
                        REQUIRES core Wifi mqtt
                        PRIV_REQUIRES SntpTime)