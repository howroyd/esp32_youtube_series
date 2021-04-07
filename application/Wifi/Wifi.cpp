#include "Wifi.h"

namespace WIFI
{

char Wifi::mac_addr_cstr[]{};

SemaphoreHandle_t Wifi::first_call_mutx{nullptr};
StaticSemaphore_t Wifi::first_call_mutx_buffer{};
bool Wifi::first_call{true};

Wifi::Wifi(void)
{
    if (!first_call_mutx) 
    {
        first_call_mutx = xSemaphoreCreateRecursiveMutexStatic(&first_call_mutx_buffer);
        configASSERT(first_call_mutx);

        configASSERT(pdPASS == xSemaphoreGive(first_call_mutx));
    }

    bool it_worked = false;

    for (int i = 0 ; i < 3 ; ++i)
        if (pdPASS == xSemaphoreTake(first_call_mutx, pdSECOND))
        {
            if (first_call)
            {
                if (ESP_OK != _get_mac()) esp_restart();
                first_call = false;
            }

            xSemaphoreGive(first_call_mutx);

            it_worked = true;

            break;
        }
        else
        {
            continue;
        }
    }

    if (!it_worked) esp_restart();
}

esp_err_t Wifi::_get_mac(void)
{
    uint8_t mac_byte_buffer[6]{};

    const esp_err_t 
        status{esp_efuse_mac_get_default(mac_byte_buffer)};

    if (ESP_OK == status)
    {
        snprintf(mac_addr_cstr, sizeof(mac_addr_cstr), 
                    "%02X%02X%02X%02X%02X%02X",
                    mac_byte_buffer[0],
                    mac_byte_buffer[1],
                    mac_byte_buffer[2],
                    mac_byte_buffer[3],
                    mac_byte_buffer[4],
                    mac_byte_buffer[5]);
    }

    return status;
}


} // namespace WIFI