set(SOURCES Bt_Common.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        PRIV_INCLUDE_DIRS include
                        REQUIRES bt nvs_flash core)