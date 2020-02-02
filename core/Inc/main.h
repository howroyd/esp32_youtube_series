#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#pragma once

#include "driver/gpio.h"

#define MEMORY_DEBUGGING

#if defined(MEMORY_DEBUGGING)
#include "multi_heap.h"
#include "esp_heap_caps.h"

//#define MEMORY_WARN_LOW
#define MEMORY_VERBOSE

#define MEMORY_LOG_INTERVAL_MS 10000
#define MEMORY_HEAP_MIN (50 * 1024) // minimum bytes before WARN_LOW triggers
#define MEMORY_STACK_MIN (256 * 2) // minimum bytes before WARN_LOW triggers
#endif

#include "Ble.h"

class Main // To ultimately become the co-ordinator if deemed necessary
{
public:
    Main(void) :
		 ble(Bt_Le::Ble::get_instance(ESP_BT_MODE_IDLE))
    {
        ;
    }

    bool setup(void);
    void run(void);

    bool start_all_tasks(void);

    Bt_Le::Ble& ble;

protected:
    static constexpr gpio_num_t signal_pin = GPIO_NUM_4;

#if defined(MEMORY_DEBUGGING)
    void log_mem(void);
#endif
}; // class Main
