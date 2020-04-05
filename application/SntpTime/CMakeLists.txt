set(SOURCES SntpTime.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        PRIV_INCLUDE_DIRS 
                        REQUIRES core Wifi
                        PRIV_REQUIRES lwip)