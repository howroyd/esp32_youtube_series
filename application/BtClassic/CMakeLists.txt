set(SOURCES Bt_Classic.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        PRIV_INCLUDE_DIRS include
                        REQUIRES Bluetooth)