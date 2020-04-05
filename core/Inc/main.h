#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#pragma once

#include "driver/gpio.h"

#include "esp_event.h"
#include "nvs_flash.h"

//#define MEMORY_DEBUGGING

#if defined(MEMORY_DEBUGGING)
#include "multi_heap.h"
#include "esp_heap_caps.h"

//#define MEMORY_WARN_LOW
#define MEMORY_VERBOSE

#define MEMORY_LOG_INTERVAL_MS 10000
#define MEMORY_HEAP_MIN (50 * 1024) // minimum bytes before WARN_LOW triggers
#define MEMORY_STACK_MIN (256 * 2)  // minimum bytes before WARN_LOW triggers
#endif

//#include "Ble.h"
#include "MqttClient.h"
#include "SntpTime.h"
#include "Gpio.h"

// --------------------------------------------------------------------------------------------------------------------
class Main final // To ultimately become the co-ordinator if deemed necessary
{
public:
    Main(void) : relay(GPIO_NUM_15), sntp{SNTP::Sntp::get_instance()} {}

    bool setup(void);
    void run(void);

    bool start_all_tasks(void);

    Gpio::gpio relay;
    //static Bt_Le::Ble &ble;
    SNTP::Sntp &sntp;
    MQTT::MqttOpenhab mqtt;

#if defined(MEMORY_DEBUGGING)
    void log_mem(void);
#endif
}; // class Main

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------