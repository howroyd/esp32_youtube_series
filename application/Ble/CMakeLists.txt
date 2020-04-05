set(SOURCES Ble.cpp 
            Gatt/Gatt_Device_Information.cpp 
            Gatt/Gatt_Hub_Information.cpp 
            Gatt/Gatt_SPP.cpp
            Gatt/Gatt_Time.cpp)

idf_component_register(SRCS ${SOURCES}
                        INCLUDE_DIRS .
                        PRIV_INCLUDE_DIRS include ./Gatt
                        REQUIRES Bluetooth)