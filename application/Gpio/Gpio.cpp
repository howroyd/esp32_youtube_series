#include "Gpio.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#define LOG_TAG "GPIO"

namespace Gpio
{

esp_err_t gpio::init(const bool state)
{
    esp_err_t status = ESP_OK;

    gpio_config_t io_conf;
	//disable interrupt
	io_conf.intr_type = GPIO_INTR_DISABLE;
	//set as output mode
	io_conf.mode = GPIO_MODE_OUTPUT;
	//bit mask of the pins that you want to set,e.g.GPIO18/19
	io_conf.pin_bit_mask = 1 << pin;
	//disable pull-down mode
	io_conf.pull_down_en = GPIO_PULLDOWN_ENABLE;
	//disable pull-up mode
	io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
	//configure GPIO with the given settings
	status &= gpio_config(&io_conf);

    if (status == ESP_OK)
    {
        status &= set(state);
    }

    return status;
}

esp_err_t gpio::set(const bool state)
{
    this->state = state;
    return gpio_set_level(pin, state);
}

bool gpio::get(void) const
{
    return state;
}

}