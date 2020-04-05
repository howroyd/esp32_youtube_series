set(SOURCES Gpio.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        REQUIRES driver)