#pragma once

#include "driver/gpio.h"
#include "esp_intr_alloc.h"

#include <cassert>
#include <cstring>
#include <utility>

namespace Gpio
{

// https://www.az-delivery.de/en/blogs/azdelivery-blog-fur-arduino-und-raspberry-pi/das-24-und-letzte-turchen
// https://www.electroschematics.com/arduino-uno-pinout/

class ArduinoPinMap
{
    using ArduinoPinMap_t = std::pair<const char*, gpio_num_t>;

    constexpr static ArduinoPinMap_t map[] = 
    {
        {"D0",  GPIO_NUM_3},
        {"D1",  GPIO_NUM_1},
        {"D2",  GPIO_NUM_26},
        {"D3",  GPIO_NUM_25},
        {"D4",  GPIO_NUM_17},
        {"D5",  GPIO_NUM_16},
        {"D6",  GPIO_NUM_27},
        {"D7",  GPIO_NUM_14},
        {"D8",  GPIO_NUM_12},
        {"D9",  GPIO_NUM_13},
        {"D10", GPIO_NUM_5},
        {"D11", GPIO_NUM_23},
        {"D12", GPIO_NUM_19},
        {"D13", GPIO_NUM_18},
        {"A0",  GPIO_NUM_39},
        {"A1",  GPIO_NUM_36},
        {"A2",  GPIO_NUM_34},
        {"A3",  GPIO_NUM_35},
        {"A4",  GPIO_NUM_4},
        {"A5",  GPIO_NUM_2},
        {"SDA", GPIO_NUM_21},
        {"SCL", GPIO_NUM_22},
        {"OD",  GPIO_NUM_0}
    };

    constexpr static auto size = sizeof(map)/sizeof(map[0]);

public:
    static constexpr gpio_num_t at(const char* key, int range = size) noexcept
    {
        return  (range == 0) ? GPIO_NUM_NC :
                    (strcmp(map[range - 1].first, key) == 0) ? map[range - 1].second :
                        at(key, range - 1);
    };
};


class GpioBase
{
protected:
    const gpio_num_t _pin;
    const bool _inverted_logic = false;
    const gpio_config_t _cfg;

public:
    constexpr GpioBase(const gpio_num_t pin,
                        const gpio_config_t& config, 
                        const bool invert_logic = false) :
        _pin{pin},
        _inverted_logic{invert_logic},
        _cfg{gpio_config_t{
                        .pin_bit_mask   = static_cast<uint64_t>(1) << _pin,
                        .mode           = config.mode,
                        .pull_up_en     = config.pull_up_en,
                        .pull_down_en   = config.pull_down_en,
                        .intr_type      = config.intr_type
            }}
    {
        assert(GPIO_IS_VALID_GPIO(pin)); // Check implemented

        if (GPIO_NUM_36 < _pin && GPIO_NUM_39 > _pin)
            assert(_cfg.pull_up_en != GPIO_PULLUP_ENABLE || 
                _cfg.pull_up_en != GPIO_PULLUP_ENABLE); // Pulling resistors not available
    }

    constexpr GpioBase(const char* arduino_pin_name,
                        const gpio_config_t& config, 
                        const bool invert_logic = false) :
        GpioBase{ArduinoPinMap::at(arduino_pin_name), config, invert_logic}
    {}

    ~GpioBase(void)
    {
        gpio_reset_pin(_pin);
    }

    [[nodiscard]] virtual bool state(void) const =0;

    [[nodiscard]] operator bool() const { return state(); }
    
    [[nodiscard]] esp_err_t init(void)
    {
        return gpio_config(&_cfg);
    }

}; // GpioBase

class GpioOutput : public GpioBase
{
    bool _state = false; // map the users wish

protected:
    constexpr GpioOutput(const gpio_num_t pin, const gpio_config_t& cfg, const bool invert) :
        GpioBase{pin, cfg, invert}
    {
        assert(GPIO_IS_VALID_OUTPUT_GPIO(pin));
    }

    constexpr GpioOutput(const char* arduino_pin_name, const gpio_config_t& cfg, const bool invert) :
        GpioOutput{ArduinoPinMap::at(arduino_pin_name), cfg, invert}
    {}

public:
    constexpr GpioOutput(const gpio_num_t pin, const bool invert = false) :
        GpioOutput{pin, 
                    gpio_config_t{
                        .pin_bit_mask   = static_cast<uint64_t>(1) << pin,
                        .mode           = GPIO_MODE_OUTPUT,     // TODO I & O, open drain
                        .pull_up_en     = GPIO_PULLUP_DISABLE,  // TODO
                        .pull_down_en   = GPIO_PULLDOWN_ENABLE, // TODO
                        .intr_type      = GPIO_INTR_DISABLE
                    }, 
                    invert}
    {}

    constexpr GpioOutput(const char* arduino_pin_name, const bool invert = false) :
        GpioOutput{ArduinoPinMap::at(arduino_pin_name), invert}
    {}

    [[nodiscard]] esp_err_t init(void)
    {
        esp_err_t status = GpioBase::init();
        if (ESP_OK == status) status |= set(false);
        return status;
    }

    esp_err_t set(const bool state)
    {
        const bool      state_to_set = _inverted_logic ? !state : state;
        const esp_err_t status       = gpio_set_level(_pin, state_to_set);
        if (ESP_OK == status) _state = state_to_set;

        return status;
    }

    [[nodiscard]] bool state(void) const 
        { return _inverted_logic ? !_state : _state; }
};

class GpioInput : public GpioBase
{
protected:
    constexpr GpioInput(const gpio_num_t pin, const gpio_config_t& cfg, const bool invert) :
        GpioBase{pin, cfg, invert}
    {}

    constexpr GpioInput(const char* arduino_pin_name, const gpio_config_t& cfg, const bool invert) :
        GpioInput{ArduinoPinMap::at(arduino_pin_name), cfg, invert}
    {}

public:
    constexpr GpioInput(const gpio_num_t pin, const bool invert = false) :
        GpioInput{pin, 
                    gpio_config_t{
                        .pin_bit_mask   = static_cast<uint64_t>(1) << pin,
                        .mode           = GPIO_MODE_INPUT,
                        .pull_up_en     = GPIO_PULLUP_DISABLE,
                        .pull_down_en   = GPIO_PULLDOWN_ENABLE,
                        .intr_type      = GPIO_INTR_DISABLE
                    }, 
                    invert}
    {}

    constexpr GpioInput(const char* arduino_pin_name, const bool invert = false) :
        GpioInput{ArduinoPinMap::at(arduino_pin_name), invert}
    {}

    [[nodiscard]] bool get(void) const
    {
        const bool pin_state = gpio_get_level(_pin) == 0 ? false : true;
        return _inverted_logic ? !pin_state : pin_state;
    }

    [[nodiscard]] bool state(void) const 
        { return get(); }
};

class GpioInterrupt : public GpioInput
{
    constexpr static esp_err_t isr_service_state_default{ESP_ERR_INVALID_STATE};
    static esp_err_t isr_service_state;

public:
    constexpr GpioInterrupt(const gpio_num_t pin, const gpio_int_type_t interrupt_type, const bool invert = false) :
        GpioInput{pin, 
                    gpio_config_t{
                        .pin_bit_mask   = static_cast<uint64_t>(1) << pin,
                        .mode           = GPIO_MODE_INPUT,
                        .pull_up_en     = GPIO_PULLUP_DISABLE,
                        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
                        .intr_type      = interrupt_type
                    }, 
                    invert}
    {
        // https://docs.espressif.com/projects/esp-idf/en/latest/esp32/api-reference/peripherals/gpio.html#_CPPv416gpio_intr_enable10gpio_num_t
        assert(!(GPIO_NUM_36 < _pin && GPIO_NUM_39 > _pin)); // Clash with WiFi driver!
    }

    constexpr GpioInterrupt(const char* arduino_pin_name, const gpio_int_type_t interrupt_type, const bool invert = false) :
        GpioInterrupt{ArduinoPinMap::at(arduino_pin_name), interrupt_type, invert}
    {}

    ~GpioInterrupt(void)
    {
        gpio_intr_disable(_pin);
        gpio_isr_handler_remove(_pin);
    }

    [[nodiscard]] esp_err_t init(gpio_isr_t isr_callback)
    {
        esp_err_t status = GpioInput::init();

        if (ESP_OK == status && isr_service_state_default == isr_service_state)
        {
            const int flags = ESP_INTR_FLAG_LOWMED | 
                (_cfg.intr_type == GPIO_INTR_POSEDGE ||
                    _cfg.intr_type == GPIO_INTR_NEGEDGE ||
                    _cfg.intr_type == GPIO_INTR_ANYEDGE
                        ? ESP_INTR_FLAG_EDGE : 0);
            isr_service_state = gpio_install_isr_service(flags);
            status |= isr_service_state;
        }

        if (ESP_OK == status)
        {
            status |= gpio_isr_handler_add(_pin, isr_callback, this);
        }

        return status;
    }
};

inline esp_err_t GpioInterrupt::isr_service_state{isr_service_state_default};

//class GpioAnalogueInput : public GpioInput;
//class GpioAnalogueOutput : public GpioOutput;

} // namespace GPIO