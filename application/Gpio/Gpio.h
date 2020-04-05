#pragma once

#include "driver/gpio.h"

namespace Gpio
{

class gpio
{
public:
    const gpio_num_t pin;

    gpio(const gpio_num_t pin) :
        pin(pin)
    {
        
    }

    esp_err_t init(const bool state = false);
    esp_err_t set(const bool state);
    bool get(void) const;

private:
    bool state = false;
};

}