#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#define pdSECOND pdMS_TO_TICKS(1000)

#include "esp_event.h"
#include "nvs_flash.h"

#include "Gpio.h"
#include "Wifi.h"
#include "SntpTime.h"
#include "Logging.h"

#include <array>

class Main final
{
    static void task_blinky(void *pvParameters);
    static TaskHandle_t h_task;

public:
    //Main(void) :
    //    sntp{SNTP::Sntp::get_instance()} {}

    esp_err_t setup(void);
    void loop(void);

protected:
    std::array<Gpio::GpioOutput, 3> multicolour_led
    {
        Gpio::GpioOutput{"D9"},   // Red
        Gpio::GpioOutput{"D11"},  // Green
        Gpio::GpioOutput{"D10"}   // Blue
    };
    
    std::array<Gpio::GpioOutput, 2> led
    {
        Gpio::GpioOutput{"D12"},  // Red
        Gpio::GpioOutput{"D13"}   // Blue
    };

    std::array<Gpio::GpioInterrupt, 2> button
    {
        Gpio::GpioInterrupt{"D3", GPIO_INTR_NEGEDGE},
        Gpio::GpioInterrupt{"D2", GPIO_INTR_NEGEDGE}
    };

    Gpio::AnalogueInput pot{"A0"};
    Gpio::AnalogueInput ldr{"A1"};
    //WIFI::Wifi wifi;
    //SNTP::Sntp& sntp;
};