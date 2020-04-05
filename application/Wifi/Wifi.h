#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/semphr.h"

#include "esp_event.h"
#include "esp_wifi.h"
#include "nvs_flash.h"

#include "lwip/err.h"
#include "lwip/sys.h"

//#include "esp_sntp.h"

#pragma once

#include "TaskCpp.h"

#define WIFI_STARTED_BIT BIT0
#define WIFI_STOPPED_BIT BIT1
#define WIFI_CONNECTED_BIT BIT2
#define WIFI_DISCONNECTED_BIT BIT3
#define WIFI_GOT_IP_BIT BIT4
#define WIFI_LOST_IP_BIT BIT5
#define WIFI_ALL_BITS (WIFI_STARTED_BIT | WIFI_STOPPED_BIT | WIFI_CONNECTED_BIT | WIFI_DISCONNECTED_BIT | WIFI_GOT_IP_BIT | WIFI_LOST_IP_BIT)
namespace WIFI
{

// --------------------------------------------------------------------------------------------------------------------
class Wifi
{
public:
    typedef void (*client_event_handler)(const EventBits_t bits);

    Wifi(void)
    {
        if (n_instances == 0)
        {
            xTaskCreate(_task, "Wifi_base", 1024 * 5, nullptr, TaskPrio_Low, &_taskhandle);
            vTaskSuspend(_taskhandle);

            _init();
        }

        ++n_instances;
    }

    ~Wifi(void)
    {
        --n_instances;

        if (n_instances == 0)
        {
            _deinit();

            vTaskDelete(_taskhandle);
            _taskhandle = nullptr;

            vSemaphoreDelete(sem_init);
            sem_init = nullptr;

            esp_event_loop_delete(event_loop);
            event_loop = nullptr;

            vEventGroupDelete(event_group);
            event_group = nullptr;
        }
    }

    static bool got_ip(void)
    {
        return (_got_ip4 | _got_ip6);
    }

    static void register_wifi_event_handler(const client_event_handler event_handler);
    static void deregister_wifi_event_handler(const client_event_handler event_handler);

private:
    static size_t n_instances;
    static esp_err_t _init(void);
    static esp_err_t _deinit(void);
    static bool _initialised, _started, _connected, _got_ip4, _got_ip6;
    static TickType_t last_start_call, last_connect_call;
    static EventGroupHandle_t event_group;
    static esp_netif_t *netif_sta;
    static wifi_init_config_t cfg;
    static esp_event_loop_handle_t event_loop;
    static constexpr esp_event_loop_args_t loop_args = {.queue_size = 16};
    static wifi_config_t wifi_config;

    static std::vector<client_event_handler> client_event_handlers;

    static SemaphoreHandle_t sem_init;

    static void _task(void *pvParameters);
    static TaskHandle_t _taskhandle;

    static void _wifi_event_handler(void *arg, esp_event_base_t event_base,
                                    int32_t event_id, void *event_data);

    static void _ip_event_handler(void *arg, esp_event_base_t event_base,
                                  int32_t event_id, void *event_data);
};
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

} // namespace WIFI
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------