#pragma once

#include "driver/gpio.h"
#include "driver/adc.h"
#include "driver/dac.h"
#include "esp_adc_cal.h"
#include "esp_intr_alloc.h"

#include "esp_log.h"

#include <cassert>
#include <cstdint>
#include <cstring>
#include <array>
#include <bitset>
#include <mutex>
#include <string_view>
#include <type_traits>
#include <utility>

namespace Gpio
{

// https://www.az-delivery.de/en/blogs/azdelivery-blog-fur-arduino-und-raspberry-pi/das-24-und-letzte-turchen
// https://www.electroschematics.com/arduino-uno-pinout/
class PinMap
{
    using ArduinoPinMap_t = std::pair<const std::string_view, const gpio_num_t>;

#if CONFIG_IDF_TARGET_ESP32
    constexpr static std::array<ArduinoPinMap_t, 23> arduino_pins = 
    {{
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
        {"A0",  GPIO_NUM_2},
        {"A1",  GPIO_NUM_4},
        {"A2",  GPIO_NUM_35},
        {"A3",  GPIO_NUM_34},
        {"A4",  GPIO_NUM_36},
        {"A5",  GPIO_NUM_39},
        {"SDA", GPIO_NUM_21},
        {"SCL", GPIO_NUM_22},
        {"OD",  GPIO_NUM_0}
    }};

    constexpr static std::array<gpio_num_t, 8> adc1_pins = 
    {{
        GPIO_NUM_32,
        GPIO_NUM_33,
        GPIO_NUM_34,
        GPIO_NUM_35,
        GPIO_NUM_36,
        GPIO_NUM_37,
        GPIO_NUM_38,
        GPIO_NUM_39
    }};

    constexpr static std::array<gpio_num_t, 8> adc2_pins = 
    {{
        //GPIO_NUM_0, // Strapping
        GPIO_NUM_2, // Strapping
        GPIO_NUM_4, // ESP-WROVER-KIT pin
        GPIO_NUM_12,
        GPIO_NUM_13,
        GPIO_NUM_14,
        //GPIO_NUM_15, // Strapping
        GPIO_NUM_25,
        GPIO_NUM_26,
        GPIO_NUM_27
    }};

    constexpr static std::array<gpio_num_t, 38> interrupt_pins = 
    {{
        GPIO_NUM_0,
        GPIO_NUM_1,
        GPIO_NUM_2,
        GPIO_NUM_3,
        GPIO_NUM_4,
        GPIO_NUM_5,
        GPIO_NUM_6,
        GPIO_NUM_7,
        GPIO_NUM_8,
        GPIO_NUM_9,
        GPIO_NUM_10,
        GPIO_NUM_11,
        GPIO_NUM_12,
        GPIO_NUM_13,
        GPIO_NUM_14,
        GPIO_NUM_15,
        GPIO_NUM_16,
        GPIO_NUM_17,
        GPIO_NUM_18,
        GPIO_NUM_19,
        GPIO_NUM_20,
        GPIO_NUM_21,
        GPIO_NUM_22,
        GPIO_NUM_23,
        GPIO_NUM_25,
        GPIO_NUM_26,
        GPIO_NUM_27,
        GPIO_NUM_28,
        GPIO_NUM_29,
        GPIO_NUM_30,
        GPIO_NUM_31,
        GPIO_NUM_32,
        GPIO_NUM_33,
        GPIO_NUM_34,
        GPIO_NUM_35,
        //GPIO_NUM_36, // Clash with WiFi
        GPIO_NUM_37,
        GPIO_NUM_38//,
        //GPIO_NUM_39 // Clash with WiFi
    }};

    constexpr static std::array<gpio_num_t, 2> dac_pins = 
    {{
        GPIO_NUM_25,
        GPIO_NUM_26
    }};

#else
#error "This code is written for ESP32 only."
#endif

public:
    [[nodiscard]] constexpr static gpio_num_t at(const std::string_view& arduino_pin_name) noexcept
    {
        for (const auto& iter : arduino_pins)
            if (iter.first == arduino_pin_name) 
                return iter.second;

        return GPIO_NUM_NC;
    }

    [[nodiscard]] constexpr static bool is_pin(const gpio_num_t pin) noexcept
    {
        return (GPIO_NUM_NC < pin && GPIO_NUM_MAX > pin) && GPIO_IS_VALID_GPIO(pin); // TODO probably don't need both the macro and our search
    }

    [[nodiscard]] constexpr static bool is_pin(const std::string_view& arduino_pin_name) noexcept
        { return is_pin(at(arduino_pin_name)); }

    [[nodiscard]] constexpr static bool is_input(const gpio_num_t pin) noexcept
        { return is_pin(pin); }

    [[nodiscard]] constexpr static bool is_input(const std::string_view& arduino_pin_name) noexcept
        { return is_input(at(arduino_pin_name)); }

    [[nodiscard]] constexpr static bool is_output(const gpio_num_t pin) noexcept
    {
        return is_pin(pin) &&
                (GPIO_NUM_NC < pin && GPIO_NUM_MAX > pin && 
                    (GPIO_NUM_34 > pin || GPIO_NUM_39 < pin)) &&
                        GPIO_IS_VALID_OUTPUT_GPIO(pin); // TODO probably don't need both the macro and our search
    }

    [[nodiscard]] constexpr static bool is_output(const std::string_view& arduino_pin_name) noexcept
        { return is_output(at(arduino_pin_name)); }

    [[nodiscard]] constexpr static bool is_input_and_output(const gpio_num_t pin) noexcept
        { return is_input(pin) && is_output(pin); }

    [[nodiscard]] constexpr static bool is_input_and_output(const std::string_view& arduino_pin_name) noexcept
        { return is_input_and_output(at(arduino_pin_name)); }

    [[nodiscard]] constexpr static bool is_input_only(const gpio_num_t pin) noexcept
        { return is_input(pin) && !is_output(pin); }

    [[nodiscard]] constexpr static bool is_input_only(const std::string_view& arduino_pin_name) noexcept
        { return is_input_only(at(arduino_pin_name)); }

    [[nodiscard]] constexpr static bool is_analogue(const gpio_num_t pin) noexcept
    {
        if (is_pin(pin))
        {
            for (const gpio_num_t avail : adc1_pins)
                if (pin == avail) 
                    return true;
            for (const gpio_num_t avail : adc2_pins)
                if (pin == avail) 
                    return true;
        }

        return false;
    }

    [[nodiscard]] constexpr static bool is_analogue(const std::string_view& arduino_pin_name) noexcept
        { return is_analogue(at(arduino_pin_name)); }

    [[nodiscard]] constexpr static bool is_interrupt(const gpio_num_t pin) noexcept
    {
        if (is_pin(pin))
            for (const gpio_num_t avail : interrupt_pins)
                if (pin == avail) 
                    return true;

        return false;
    }

    [[nodiscard]] constexpr static bool is_interrupt(const std::string_view& arduino_pin_name) noexcept
        { return is_interrupt(at(arduino_pin_name)); }

    [[nodiscard]] constexpr static bool is_adc1(const gpio_num_t pin) noexcept
    {
        if (is_pin(pin))
        {
            for (const gpio_num_t avail : adc1_pins)
                if (pin == avail) 
                    return true;
        }

        return false;
    }

    [[nodiscard]] constexpr static bool is_adc1(const std::string_view& arduino_pin_name) noexcept
        { return is_adc1(at(arduino_pin_name)); }

    [[nodiscard]] constexpr static bool is_adc2(const gpio_num_t pin) noexcept
    {
        if (is_pin(pin))
        {
            for (const gpio_num_t avail : adc2_pins)
                if (pin == avail) 
                    return true;
        }

        return false;
    }

    [[nodiscard]] constexpr static bool is_adc2(const std::string_view& arduino_pin_name) noexcept
        { return is_adc2(at(arduino_pin_name)); }

    [[nodiscard]] constexpr static bool is_dac(const gpio_num_t pin) noexcept
    {
        if (is_pin(pin))
        {
            for (const gpio_num_t avail : dac_pins)
                if (pin == avail) 
                    return true;
        }

        return false;
    }

    [[nodiscard]] constexpr static bool is_dac(const std::string_view& arduino_pin_name) noexcept
        { return is_dac(at(arduino_pin_name)); }

    PinMap(void) = delete; // Not constructable, compile time only
};

class GpioBase
{
    static std::mutex _in_use_mutx;
    static std::bitset<GPIO_NUM_MAX> _in_use;

protected:
    const gpio_num_t    _pin;
    const bool          _inverted_logic = false;
    const gpio_config_t _cfg;

    [[nodiscard]] esp_err_t lock_pin(void) noexcept
        { return _lock_pin(_pin); }

    esp_err_t unlock_pin(void) noexcept
        { return _unlock_pin(_pin); }

public:
    constexpr static bool is_valid_pin(const gpio_num_t pin) noexcept
        { return PinMap::is_pin(pin); }
    constexpr static bool is_valid_pin(const std::string_view& arduino_pin_name) noexcept
        { return is_valid_pin(PinMap::at(arduino_pin_name)); }

    constexpr GpioBase(const gpio_num_t pin,
                        const gpio_config_t& config, 
                        const bool invert_logic = false) noexcept :
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
        assert(is_valid_pin(pin));

        assert(!(PinMap::is_input_only(pin) && 
                    (_cfg.pull_up_en == GPIO_PULLUP_ENABLE || _cfg.pull_up_en == GPIO_PULLUP_ENABLE)));
    }

    constexpr GpioBase(const std::string_view& arduino_pin_name,
                        const gpio_config_t& config, 
                        const bool invert_logic = false) noexcept :
        GpioBase{PinMap::at(arduino_pin_name), config, invert_logic}
    {}

    ~GpioBase(void) noexcept
        { deinit(); }

    [[nodiscard]] virtual bool                        state(void) const noexcept =0;

    [[nodiscard]] operator bool() const noexcept
        { return state(); }
    
    [[nodiscard]] esp_err_t                           init(void) noexcept
    {
        const esp_err_t status = lock_pin();
        if (ESP_OK == status)
            return gpio_config(&_cfg);

        return status;
    }

    esp_err_t                                         deinit(void) noexcept
    {
        gpio_reset_pin(_pin);
        unlock_pin();

        return ESP_OK;
    }

    [[nodiscard]] constexpr const gpio_num_t&         pin(void) const noexcept
        { return _pin; }

    [[nodiscard]] constexpr const bool&               inverted_logic(void) const noexcept
        { return _inverted_logic; }

    [[nodiscard]] constexpr const uint64_t&           cfg_pin_bit_mask(void) const noexcept
        { return _cfg.pin_bit_mask; }

    [[nodiscard]] constexpr const gpio_mode_t&        cfg_mode(void) const noexcept
        { return _cfg.mode; }

    [[nodiscard]] constexpr const gpio_pullup_t&      cfg_pull_up_en(void) const noexcept
        { return _cfg.pull_up_en; }

    [[nodiscard]] constexpr const gpio_pulldown_t&    cfg_pull_down_en(void) const noexcept
        { return _cfg.pull_down_en; }

    [[nodiscard]] constexpr const gpio_int_type_t&    cfg_intr_type(void) const noexcept
        { return _cfg.intr_type; }

private:
    [[nodiscard]] static esp_err_t _lock_pin(const gpio_num_t pin) noexcept
    {
        const std::lock_guard<std::mutex> lock(_in_use_mutx);

        if (!_in_use[pin])
        {
            _in_use.set(pin);

            return ESP_OK;
        }

        return ESP_ERR_INVALID_STATE;
    }

    static esp_err_t _unlock_pin(const gpio_num_t pin) noexcept
    {
        const std::lock_guard<std::mutex> lock(_in_use_mutx);

        if (_in_use[pin])
        {
            _in_use.reset(pin);

            return ESP_OK;
        }

        return ESP_ERR_INVALID_STATE;
    }
}; // GpioBase

inline std::bitset<GPIO_NUM_MAX> GpioBase::_in_use;
inline std::mutex                GpioBase::_in_use_mutx;

class GpioOutput : public GpioBase
{
    bool _state = false; // map the users wish

protected:
    constexpr GpioOutput(const gpio_num_t pin, 
                            const gpio_config_t& cfg, 
                            const bool invert) noexcept :
        GpioBase{pin, cfg, invert}
    { assert(is_valid_pin(pin)); }

    constexpr GpioOutput(const std::string_view& arduino_pin_name, 
                            const gpio_config_t& cfg, 
                            const bool invert) noexcept :
        GpioOutput{PinMap::at(arduino_pin_name), cfg, invert}
    {}

public:
    constexpr static bool is_valid_pin(const gpio_num_t pin) noexcept
        { return GpioBase::is_valid_pin(pin) && PinMap::is_output(pin); }
    constexpr static bool is_valid_pin(const std::string_view& arduino_pin_name) noexcept
        { return is_valid_pin(PinMap::at(arduino_pin_name)); }

    constexpr GpioOutput(const gpio_num_t pin, const bool invert = false) noexcept :
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

    constexpr GpioOutput(const std::string_view& arduino_pin_name, const bool invert = false) noexcept :
        GpioOutput{PinMap::at(arduino_pin_name), invert}
    {}

    [[nodiscard]] esp_err_t init(void) noexcept
    {
        esp_err_t status = GpioBase::init();
        if (ESP_OK == status) status |= set(false);
        return status;
    }

    esp_err_t set(const bool state) noexcept
    {
        const bool      state_to_set = _inverted_logic ? !state : state;
        const esp_err_t status       = gpio_set_level(_pin, state_to_set);
        if (ESP_OK == status) _state = state_to_set;

        return status;
    }

    [[nodiscard]] bool state(void) const noexcept 
        { return _inverted_logic ? !_state : _state; }
};

class GpioInput : public GpioBase
{
protected:
    constexpr GpioInput(const gpio_num_t pin, 
                        const gpio_config_t& cfg, 
                        const bool invert = false) noexcept :
        GpioBase{pin, cfg, invert}
    { assert(is_valid_pin(pin)); }

    constexpr GpioInput(const std::string_view& arduino_pin_name, 
                        const gpio_config_t& cfg, 
                        const bool invert = false) noexcept :
        GpioInput{PinMap::at(arduino_pin_name), cfg, invert}
    {}

public:
    constexpr static bool is_valid_pin(const gpio_num_t pin) noexcept
        { return GpioBase::is_valid_pin(pin) && PinMap::is_input(pin); }
    constexpr static bool is_valid_pin(const std::string_view& arduino_pin_name) noexcept
        { return is_valid_pin(PinMap::at(arduino_pin_name)); }

    constexpr GpioInput(const gpio_num_t pin, const bool invert = false) noexcept :
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

    constexpr GpioInput(const std::string_view& arduino_pin_name, 
                        const bool invert = false) noexcept :
        GpioInput{PinMap::at(arduino_pin_name), invert}
    {}

    [[nodiscard]] bool get(void) const noexcept
    {
        const bool pin_state = gpio_get_level(_pin) == 0 ? false : true;
        return _inverted_logic ? !pin_state : pin_state;
    }

    [[nodiscard]] bool state(void) const noexcept
        { return get(); }
};

class GpioInterrupt final : public GpioInput
{
    constexpr static esp_err_t isr_service_state_default{ESP_ERR_INVALID_STATE};
    static esp_err_t isr_service_state;

public:
    constexpr static bool is_valid_pin(const gpio_num_t pin) noexcept
        { return GpioInput::is_valid_pin(pin) && PinMap::is_interrupt(pin); }
    constexpr static bool is_valid_pin(const std::string_view& arduino_pin_name) noexcept
        { return is_valid_pin(PinMap::at(arduino_pin_name)); }

    constexpr GpioInterrupt(const gpio_num_t pin, 
                            const gpio_int_type_t interrupt_type) noexcept :
        GpioInput{pin, 
                    gpio_config_t{
                        .pin_bit_mask   = static_cast<uint64_t>(1) << pin,
                        .mode           = GPIO_MODE_INPUT,
                        .pull_up_en     = GPIO_PULLUP_DISABLE,
                        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
                        .intr_type      = interrupt_type
                    }}
    { assert(is_valid_pin(pin)); }

    constexpr GpioInterrupt(const std::string_view& arduino_pin_name, 
                            const gpio_int_type_t interrupt_type) noexcept :
        GpioInterrupt{PinMap::at(arduino_pin_name), interrupt_type}
    {}

    ~GpioInterrupt(void) noexcept
    {
        deinit();
    }

    [[nodiscard]] esp_err_t init(gpio_isr_t isr_callback, void* isr_args = nullptr) noexcept
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
            status |= gpio_isr_handler_add(_pin, isr_callback, isr_args ? isr_args : this);
        }

        return status;
    }

    esp_err_t deinit(void) noexcept
    {
        gpio_intr_disable(_pin);
        gpio_isr_handler_remove(_pin);
        return GpioInput::deinit();
    }
};

inline esp_err_t GpioInterrupt::isr_service_state{isr_service_state_default};

class AnalogueInput final : public GpioInput
{
    constexpr static adc_bits_width_t  width_default{ADC_WIDTH_BIT_12};
    constexpr static adc_atten_t       atten_default{ADC_ATTEN_DB_0};
    constexpr static uint32_t          n_samples_default{10};
    static           bool              _two_point_supported, _vref_supported;

    enum class Adc_num_t : int
    {
        ADC_1 = 1,
        ADC_2 = 2,
        ADC_MAX = -1
    } const _adc_num{Adc_num_t::ADC_MAX};

    const adc_channel_t     _channel;
    const adc_bits_width_t  _width{width_default};
    const adc_atten_t       _atten{atten_default};
    const adc_unit_t        _unit{ADC_UNIT_1}; // ESP32 only supports unit 1

    const adc1_channel_t    _adc1_channel; // TODO std::variant instead???
    const adc2_channel_t    _adc2_channel; // TODO std::variant instead???

    esp_adc_cal_characteristics_t _adc_chars{};

    uint32_t _vref{1100};

    const   float _lpf_k{0.4};
    mutable float _lpf_last{0};

public:
    constexpr static bool is_valid_pin(const gpio_num_t pin) noexcept
        { return GpioInput::is_valid_pin(pin) && PinMap::is_analogue(pin); }
    constexpr static bool is_valid_pin(const std::string_view& arduino_pin_name) noexcept
        { return is_valid_pin(PinMap::at(arduino_pin_name)); }

    constexpr AnalogueInput(const gpio_num_t pin,
                                const adc_bits_width_t width,
                                const adc_atten_t attenuation) noexcept :
        GpioInput{pin, 
                    gpio_config_t{
                        .pin_bit_mask   = static_cast<uint64_t>(1) << pin,
                        .mode           = GPIO_MODE_INPUT,
                        .pull_up_en     = GPIO_PULLUP_DISABLE,
                        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
                        .intr_type      = GPIO_INTR_DISABLE
                    }},
        _adc_num{pin_to_adc_num(pin)},
        _channel{pin_to_adc_channel(pin)},
        _width{width},
        _atten{attenuation},
        _adc1_channel{pin_to_adc1_channel(pin)},
        _adc2_channel{pin_to_adc2_channel(pin)}
    { 
        assert(is_valid_pin(pin));
        assert(Adc_num_t::ADC_MAX != _adc_num);
        assert(ADC_CHANNEL_MAX > _channel);
        assert(ADC_WIDTH_MAX > _width);
        assert(ADC_ATTEN_MAX > _atten);
        switch (_adc_num)
        {
        case Adc_num_t::ADC_1:
            assert(ADC1_CHANNEL_MAX > _adc1_channel);
            break;
        case Adc_num_t::ADC_2:
            assert(ADC2_CHANNEL_MAX > _adc2_channel);
            break;
        default:
            assert(false);
        };
    }

    constexpr AnalogueInput(const gpio_num_t pin) noexcept :
        AnalogueInput{pin, width_default, atten_default}
    {}

    constexpr AnalogueInput(const std::string_view& arduino_pin_name) noexcept :
        AnalogueInput{PinMap::at(arduino_pin_name), width_default, atten_default}
    {}

    explicit constexpr AnalogueInput(const gpio_num_t pin,
                                            const adc_bits_width_t width) noexcept :
        AnalogueInput{pin, width, atten_default}
    {}

    explicit constexpr AnalogueInput(const gpio_num_t pin,
                                            const adc_atten_t attenuation) noexcept :
        AnalogueInput{pin, width_default, attenuation}
    {}

    constexpr AnalogueInput(const std::string_view& arduino_pin_name,
                                const adc_bits_width_t width,
                                const adc_atten_t attenuation) noexcept :
        AnalogueInput{PinMap::at(arduino_pin_name), width, attenuation}
    {}

    [[nodiscard]] esp_err_t                         init(void) noexcept
    {
        check_efuse();

        esp_err_t status{lock_pin()}; // Unlock handled by base destructor
        
        if (ESP_OK == status)
        {
            switch (_adc_num)
            {
            case Adc_num_t::ADC_1:
                if (ESP_OK == status)
                    status |= adc1_config_width(_width);
                
                if (ESP_OK == status)
                    status |= adc1_config_channel_atten(_adc1_channel, _atten);
                break;
            case Adc_num_t::ADC_2:
                if (ESP_OK == status)
                    status |= adc2_config_channel_atten(_adc2_channel, _atten);
                break;
            default:
                status = ESP_FAIL;
                break;
            };

            const esp_adc_cal_value_t 
            val_type = esp_adc_cal_characterize(_unit, 
                                                _atten, 
                                                _width, 
                                                _vref, 
                                                &_adc_chars); // TODO vref

            (void)val_type;
        }

        return status;
    }

    [[nodiscard]] uint32_t                          get(const uint32_t n_samples = n_samples_default) const noexcept
    {
        esp_err_t   status{ESP_OK}; 
        size_t      n_fails = 0;
        int         cumulative_result = 0;

        switch (_adc_num)
        {
        case Adc_num_t::ADC_1:
            for (uint32_t i = 0 ; i < n_samples ; ++i)
            {
                cumulative_result += adc1_get_raw(_adc1_channel);
            }
            break;
        case Adc_num_t::ADC_2:
        {
            int temp_result = 0;
            for (uint32_t i = 0 ; i < n_samples ; ++i)
            {
                status = adc2_get_raw(_adc2_channel, _width, &temp_result);
                if (ESP_OK == status) cumulative_result += temp_result;
                else ++n_fails;
            }
            break;
        }
        default:
            n_fails = n_samples;
            status = ESP_FAIL;
            break;
        };

        const int n_samples_read = n_samples - n_fails;
        
        if (0 < n_samples_read)
        {
            int mean_result = cumulative_result / n_samples_read;
            return esp_adc_cal_raw_to_voltage(mean_result, &_adc_chars);
        }

        return 0;
    }

    [[nodiscard]] uint32_t                          get_filtered(const uint32_t n_samples = n_samples_default) const noexcept
    {
        const float new_val = static_cast<float>(get(n_samples));
        const float filtered_val = _lpf_last + _lpf_k * (new_val - _lpf_last);
        _lpf_last = filtered_val;
        return filtered_val;
    }

    [[nodiscard]] bool                              state(void) const noexcept
        { return get() > _vref; }

    [[nodiscard]] constexpr const bool&             two_point_supported(void) const noexcept    
        { return _two_point_supported; }

    [[nodiscard]] constexpr const bool&             vref_supported(void) const noexcept
        { return _vref_supported; }

    [[nodiscard]] constexpr int                     adc_num_in_idf(void) const noexcept
        { return static_cast<std::underlying_type<Adc_num_t>::type>(_adc_num); }

    [[nodiscard]] constexpr const adc_channel_t&    channel(void) const noexcept
        { return _channel; }

    [[nodiscard]] constexpr const adc_bits_width_t& width(void) const noexcept
        { return _width; }

    [[nodiscard]] constexpr const adc_atten_t&      attenuation(void) const noexcept
        { return _atten; }

    [[nodiscard]] constexpr const adc_unit_t&       unit(void) const noexcept
        { return _unit; }

private:
    static void check_efuse(void) noexcept
    {
        if (ESP_OK == esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP)) _two_point_supported = true;
        else _two_point_supported = false;

        if (ESP_OK == esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_VREF)) _vref_supported = true;
        else _vref_supported = false;
    }

    static constexpr adc1_channel_t pin_to_adc1_channel(const gpio_num_t pin) noexcept
    {
        switch (pin)
        {
        case GPIO_NUM_36:
            return ADC1_CHANNEL_0;
        case GPIO_NUM_37:
            return ADC1_CHANNEL_1;
        case GPIO_NUM_38:
            return ADC1_CHANNEL_2;
        case GPIO_NUM_39:
            return ADC1_CHANNEL_3;
        case GPIO_NUM_32:
            return ADC1_CHANNEL_4;
        case GPIO_NUM_33:
            return ADC1_CHANNEL_5;
        case GPIO_NUM_34:
            return ADC1_CHANNEL_6;
        case GPIO_NUM_35:
            return ADC1_CHANNEL_7;
        default:
            return ADC1_CHANNEL_MAX;
        };
    }

    static constexpr adc2_channel_t pin_to_adc2_channel(const gpio_num_t pin) noexcept
    {
        switch (pin)
        {
        case GPIO_NUM_4:
            return ADC2_CHANNEL_0;
        case GPIO_NUM_0:
            return ADC2_CHANNEL_1;
        case GPIO_NUM_2:
            return ADC2_CHANNEL_2;
        case GPIO_NUM_15:
            return ADC2_CHANNEL_3;
        case GPIO_NUM_13:
            return ADC2_CHANNEL_4;
        case GPIO_NUM_12:
            return ADC2_CHANNEL_5;
        case GPIO_NUM_14:
            return ADC2_CHANNEL_6;
        case GPIO_NUM_27:
            return ADC2_CHANNEL_7;
        case GPIO_NUM_25:
            return ADC2_CHANNEL_8;
        case GPIO_NUM_26:
            return ADC2_CHANNEL_9;
        default:
            return ADC2_CHANNEL_MAX;
        };
    }

    static constexpr Adc_num_t pin_to_adc_num(const gpio_num_t pin) noexcept
    {
        const adc1_channel_t adc1_ch = pin_to_adc1_channel(pin);
        const adc2_channel_t adc2_ch = pin_to_adc2_channel(pin);

        //if (ADC1_CHANNEL_MAX == adc1_ch && ADC2_CHANNEL_MAX == adc2_ch)
        //    return Adc_num_t::ADC_MAX;
        //if (ADC1_CHANNEL_MAX != adc1_ch && ADC2_CHANNEL_MAX != adc2_ch)
        //    return Adc_num_t::ADC_MAX;

        if (!((ADC1_CHANNEL_MAX != adc1_ch && ADC2_CHANNEL_MAX != adc2_ch) ||
            (ADC1_CHANNEL_MAX == adc1_ch && ADC2_CHANNEL_MAX == adc2_ch)))
        {
            // Not both valid channels and not bot invalid channels (effectively logical XOR)
            if (PinMap::is_adc1(pin) && ADC1_CHANNEL_MAX != adc1_ch)
                return Adc_num_t::ADC_1;
            else if (PinMap::is_adc2(pin) && ADC2_CHANNEL_MAX != adc2_ch)
                return Adc_num_t::ADC_2;
            // NOTE If we get here something is wrong with the logic
        }
        return Adc_num_t::ADC_MAX;
    }

    static constexpr adc_channel_t pin_to_adc_channel(const gpio_num_t pin) noexcept
    {
        switch (pin_to_adc_num(pin))
        {
        case Adc_num_t::ADC_1:
            return static_cast<adc_channel_t>(pin_to_adc1_channel(pin));
        case Adc_num_t::ADC_2:
            return static_cast<adc_channel_t>(pin_to_adc2_channel(pin));
        default:
            break;
        };
        return ADC_CHANNEL_MAX;
    }
};

inline bool AnalogueInput::_two_point_supported{false}, 
            AnalogueInput::_vref_supported{false};

class DacOutput : public GpioOutput
{
    /*const*/ dac_channel_t _channel{DAC_CHANNEL_MAX};

    uint8_t _output = 0;
    constexpr static uint32_t vref = 3300;

public:
    constexpr static bool is_valid_pin(const gpio_num_t pin) noexcept
        { return GpioOutput::is_valid_pin(pin) && PinMap::is_dac(pin); }
    constexpr static bool is_valid_pin(const std::string_view& arduino_pin_name) noexcept
        { return is_valid_pin(PinMap::at(arduino_pin_name)); }

    constexpr DacOutput(const gpio_num_t pin) noexcept :
        GpioOutput{pin, 
                    gpio_config_t{
                        .pin_bit_mask   = static_cast<uint64_t>(1) << pin,
                        .mode           = GPIO_MODE_INPUT,
                        .pull_up_en     = GPIO_PULLUP_DISABLE,
                        .pull_down_en   = GPIO_PULLDOWN_DISABLE,
                        .intr_type      = GPIO_INTR_DISABLE
                    }, false},
        _channel{pin_to_dac_channel(pin)}
    { 
        assert(is_valid_pin(pin));
        assert(DAC_CHANNEL_MAX != _channel);
    }

    constexpr DacOutput(const std::string_view& arduino_pin_name) noexcept :
        DacOutput{PinMap::at(arduino_pin_name)}
    {}

    ~DacOutput(void) noexcept
        { disable(); }

    [[nodiscard]] esp_err_t init(void) noexcept
    {
        esp_err_t status{lock_pin()}; // Unlock handled by base destructor
        if (ESP_OK == status)
            status |= enable();
        if (ESP_OK == status) 
            status |= set(0);
        return status;
    }

    [[nodiscard]] esp_err_t enable(void) noexcept
        { return dac_output_enable(_channel); }

    esp_err_t disable(void) noexcept
        { return dac_output_disable(_channel); }

    [[nodiscard]] esp_err_t set(const uint8_t val) noexcept
    { 
        const esp_err_t status = dac_output_voltage(_channel, val);
        if (ESP_OK == status)
            _output = val;
        return status;
    }

    [[nodiscard]] esp_err_t set_mv(const uint32_t mv) noexcept
        { return set(mv_to_val(mv)); }

    [[nodiscard]] constexpr const uint8_t& get(void) const noexcept
        { return _output; }

    [[nodiscard]] constexpr uint32_t get_mv(void) const noexcept
        { return val_to_mv(_output); }

private:
    [[nodiscard]] static constexpr dac_channel_t pin_to_dac_channel(const gpio_num_t pin) noexcept
    {
        switch (pin)
        {
        case GPIO_NUM_25:
            return DAC_CHANNEL_1;
        case GPIO_NUM_26:
            return DAC_CHANNEL_2;
        default:
            return DAC_CHANNEL_MAX;
        };
    }

    [[nodiscard]] static constexpr uint32_t val_to_mv(const uint8_t val) noexcept
    {
        return (vref * static_cast<uint32_t>(val)) / static_cast<uint32_t>(UINT8_MAX);
    }

    [[nodiscard]] static constexpr uint8_t mv_to_val(const uint32_t mv) noexcept
    {
        return (mv * static_cast<uint32_t>(UINT8_MAX)) / vref;
    }
};

} // namespace GPIO