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
public:
    //Main(void) :
    //    sntp{SNTP::Sntp::get_instance()} {}

    esp_err_t setup(void);
    void loop(void);

    Gpio::GpioOutput led{GPIO_NUM_27, true};
    std::array<Gpio::GpioOutput, 3> multicolour_led{Gpio::GpioOutput{"D9"}, Gpio::GpioOutput{"D11"}, Gpio::GpioOutput{"D10"}}; // RGB
    Gpio::AnalogueInput pot{"A1"};
    //WIFI::Wifi wifi;
    //SNTP::Sntp& sntp;
};