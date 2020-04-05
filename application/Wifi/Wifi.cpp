#include "Wifi.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#define LOG_TAG "WIFI"

#define DEFAULT_DELAY (pdSECOND / 20)

namespace WIFI
{

size_t Wifi::n_instances = 0;
bool Wifi::_initialised = false;
bool Wifi::_started = false;
bool Wifi::_connected = false;
bool Wifi::_got_ip4 = false;
bool Wifi::_got_ip6 = false;
TickType_t Wifi::last_start_call = 0;
TickType_t Wifi::last_connect_call = 0;
EventGroupHandle_t Wifi::event_group = nullptr;
esp_netif_t *Wifi::netif_sta = nullptr;
wifi_init_config_t Wifi::cfg{};
esp_event_loop_handle_t Wifi::event_loop{};
wifi_config_t Wifi::wifi_config{.sta{CONFIG_WIFI_SSID, CONFIG_WIFI_PASS}};
SemaphoreHandle_t Wifi::sem_init = nullptr;
TaskHandle_t Wifi::_taskhandle = nullptr;
std::vector<Wifi::client_event_handler> Wifi::client_event_handlers;

// --------------------------------------------------------------------------------------------------------------------
void Wifi::_task(void *pvParameters)
{
    EventBits_t bits{0};

    vTaskDelay(DEFAULT_DELAY);

    while (true)
    {
        if (!_initialised)
        {
            _init();
        }
        else if (!_started)
        {
            static constexpr const TickType_t period = pdSECOND * 10;

            if ((last_start_call == 0) || ((xTaskGetTickCount() - last_start_call) > period))
            {
                ESP_LOGD(LOG_TAG, "Starting");
                vTaskDelay(DEFAULT_DELAY);

                esp_wifi_start();
                last_start_call = xTaskGetTickCount();
            }
            else
            {
                vTaskDelay(xTaskGetTickCount() - last_start_call);
            }
        }
        else if (!_connected)
        {
            static constexpr const TickType_t period = pdSECOND * 10;

            if ((last_connect_call == 0) || ((xTaskGetTickCount() - last_connect_call) > period))
            {
                wifi_scan();
                
                ESP_LOGD(LOG_TAG, "Connecting");
                vTaskDelay(DEFAULT_DELAY);

                esp_wifi_connect();
                last_connect_call = xTaskGetTickCount();
            }
            else
            {
                vTaskDelay(xTaskGetTickCount() - last_connect_call);
            }
        }
        else
        {
            bits = xEventGroupWaitBits(event_group,
                                       WIFI_ALL_BITS,
                                       pdTRUE,
                                       pdFALSE,
                                       portMAX_DELAY);
            ESP_LOGD(LOG_TAG, "Event group triggered");
            vTaskDelay(DEFAULT_DELAY);

            if (bits & WIFI_GOT_IP_BIT || bits & WIFI_LOST_IP_BIT)
                for (auto& func : client_event_handlers)
                    func(bits);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
esp_err_t Wifi::_init(void)
{
    esp_err_t status = ESP_OK;

    if (sem_init == nullptr)
    {
        sem_init = xSemaphoreCreateBinary();
        xSemaphoreGive(sem_init);
    }

    if (xSemaphoreTake(sem_init, pdSECOND * 60) == pdTRUE)
    {
        if (_initialised == false)
        {
            ESP_LOGD(LOG_TAG, "Initialising");
            vTaskDelay(DEFAULT_DELAY);

            // Create the event group that the HAL will send notifications to
            if (!event_group)
            {
                ESP_LOGD(LOG_TAG, "Creating event group");
                vTaskDelay(DEFAULT_DELAY);
                event_group = xEventGroupCreate();
            }

            // Create our event loop that we will use to notify class instances
            if (!event_loop)
            {
                ESP_LOGD(LOG_TAG, "Creating event loop");
                vTaskDelay(DEFAULT_DELAY);
                ESP_ERROR_CHECK(esp_event_loop_create(&loop_args, &event_loop));
            }

            // Initialize the underlying TCP/IP stack
            ESP_LOGD(LOG_TAG, "Initialising netif TCP/IP Stack");
            vTaskDelay(DEFAULT_DELAY);
            ESP_ERROR_CHECK(esp_netif_init());

            // Creates default WIFI STA
            if (!netif_sta)
            {
                ESP_LOGD(LOG_TAG, "Creating default netif STA");
                vTaskDelay(DEFAULT_DELAY);
                netif_sta = esp_netif_create_default_wifi_sta();
            }

            cfg = WIFI_INIT_CONFIG_DEFAULT();

            // Alloc resource for WiFi driver
            ESP_LOGD(LOG_TAG, "Allocating resource for WiFi driver");
            vTaskDelay(DEFAULT_DELAY);
            ESP_ERROR_CHECK(esp_wifi_init(&cfg));

            // Register our event handler callback with the HAL
            ESP_LOGD(LOG_TAG, "Registering event handlers");
            vTaskDelay(DEFAULT_DELAY);
            ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler, NULL));
            ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, ESP_EVENT_ANY_ID, &_ip_event_handler, NULL));

            // Set the WiFi operating mode
            ESP_LOGD(LOG_TAG, "Setting operating mode");
            vTaskDelay(DEFAULT_DELAY);
            ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));

            // Set the configuration of the ESP32 STA
            ESP_LOGD(LOG_TAG, "Setting config");
            vTaskDelay(DEFAULT_DELAY);
            ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));

            ESP_LOGD(LOG_TAG, "Starting task");
            vTaskDelay(DEFAULT_DELAY);
            vTaskResume(_taskhandle);

            _initialised = true;
        }

        xSemaphoreGive(sem_init);
    }
    else
    {
        status = ESP_ERR_TIMEOUT;
    }

    return status;
}

// --------------------------------------------------------------------------------------------------------------------
esp_err_t Wifi::_deinit(void)
{
    esp_err_t status = ESP_OK;

    if (sem_init == nullptr)
    {
        sem_init = xSemaphoreCreateBinary();
        xSemaphoreGive(sem_init);
    }

    if (xSemaphoreTake(sem_init, pdSECOND * 60) == pdTRUE)
    {
        if (_initialised == true)
        {
            ESP_LOGD(LOG_TAG, "Stopping task");
            vTaskDelay(DEFAULT_DELAY);
            vTaskSuspend(_taskhandle);

            if (_connected)
            {
                ESP_LOGD(LOG_TAG, "Disconnecting");
                vTaskDelay(DEFAULT_DELAY);

                esp_wifi_disconnect();

                while (_connected)
                    vTaskDelay(DEFAULT_DELAY * 5);
            }

            if (_started)
            {
                ESP_LOGD(LOG_TAG, "Stopping");
                vTaskDelay(DEFAULT_DELAY);

                esp_wifi_stop();

                while (_started)
                    vTaskDelay(DEFAULT_DELAY * 5);
            }

            ESP_LOGD(LOG_TAG, "Unregistering event handlers");
            vTaskDelay(DEFAULT_DELAY);
            esp_event_handler_unregister(IP_EVENT, ESP_EVENT_ANY_ID, &_ip_event_handler);
            esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &_wifi_event_handler);

            ESP_LOGD(LOG_TAG, "Deinitialising WiFi driver");
            vTaskDelay(DEFAULT_DELAY);
            esp_wifi_deinit();

            _initialised = false;
        }

        xSemaphoreGive(sem_init);
    }
    else
    {
        status = ESP_ERR_TIMEOUT;
    }

    return status;
}

// --------------------------------------------------------------------------------------------------------------------
void Wifi::register_wifi_event_handler(const client_event_handler event_handler)
{
    bool match_found{false};

    for (auto iter = client_event_handlers.begin(); iter != client_event_handlers.end(); ++iter)
    {
        if (event_handler == *iter)
        {
            // Match found for this handle
            match_found = true;
            break;
        }
    }
    if (!match_found)
        client_event_handlers.push_back(event_handler);
}

// --------------------------------------------------------------------------------------------------------------------
void Wifi::deregister_wifi_event_handler(const client_event_handler event_handler)
{
    for (auto iter = client_event_handlers.begin(); iter != client_event_handlers.end(); ++iter)
    {
        if (event_handler == *iter)
        {
            // Match found for this handle
            client_event_handlers.erase(iter);
        }
    }
}

// --------------------------------------------------------------------------------------------------------------------
void Wifi::wifi_scan(void)
{
    uint16_t number = 20;
    wifi_ap_record_t ap_info[number]{};
    uint16_t ap_count = 0;

    esp_wifi_scan_start(NULL, true);
    esp_wifi_scan_get_ap_records(&number, ap_info);
    esp_wifi_scan_get_ap_num(&ap_count);
    ESP_LOGI(LOG_TAG, "Total APs scanned = %u", ap_count);

    for (int i = 0; (i < number) && (i < ap_count); ++i) 
    {
        ESP_LOGI(LOG_TAG, "SSID \t\t%s", ap_info[i].ssid);
        ESP_LOGI(LOG_TAG, "RSSI \t\t%d", ap_info[i].rssi);
        print_auth_mode(ap_info[i].authmode);
        if (ap_info[i].authmode != WIFI_AUTH_WEP) {
            print_cipher_type(ap_info[i].pairwise_cipher, ap_info[i].group_cipher);
        }
        ESP_LOGI(LOG_TAG, "Channel \t\t%d\n", ap_info[i].primary);
    }
}

// --------------------------------------------------------------------------------------------------------------------
void Wifi::print_auth_mode(int authmode)
{
    switch (authmode) {
    case WIFI_AUTH_OPEN:
        ESP_LOGI(LOG_TAG, "Authmode \tWIFI_AUTH_OPEN");
        break;
    case WIFI_AUTH_WEP:
        ESP_LOGI(LOG_TAG, "Authmode \tWIFI_AUTH_WEP");
        break;
    case WIFI_AUTH_WPA_PSK:
        ESP_LOGI(LOG_TAG, "Authmode \tWIFI_AUTH_WPA_PSK");
        break;
    case WIFI_AUTH_WPA2_PSK:
        ESP_LOGI(LOG_TAG, "Authmode \tWIFI_AUTH_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA_WPA2_PSK:
        ESP_LOGI(LOG_TAG, "Authmode \tWIFI_AUTH_WPA_WPA2_PSK");
        break;
    case WIFI_AUTH_WPA2_ENTERPRISE:
        ESP_LOGI(LOG_TAG, "Authmode \tWIFI_AUTH_WPA2_ENTERPRISE");
        break;
    default:
        ESP_LOGI(LOG_TAG, "Authmode \tWIFI_AUTH_UNKNOWN");
        break;
    }

}

// --------------------------------------------------------------------------------------------------------------------
void Wifi::print_cipher_type(int pairwise_cipher, int group_cipher)
{
    switch (pairwise_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(LOG_TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(LOG_TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(LOG_TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(LOG_TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(LOG_TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(LOG_TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    default:
        ESP_LOGI(LOG_TAG, "Pairwise Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }

    switch (group_cipher) {
    case WIFI_CIPHER_TYPE_NONE:
        ESP_LOGI(LOG_TAG, "Group Cipher \tWIFI_CIPHER_TYPE_NONE");
        break;
    case WIFI_CIPHER_TYPE_WEP40:
        ESP_LOGI(LOG_TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP40");
        break;
    case WIFI_CIPHER_TYPE_WEP104:
        ESP_LOGI(LOG_TAG, "Group Cipher \tWIFI_CIPHER_TYPE_WEP104");
        break;
    case WIFI_CIPHER_TYPE_TKIP:
        ESP_LOGI(LOG_TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP");
        break;
    case WIFI_CIPHER_TYPE_CCMP:
        ESP_LOGI(LOG_TAG, "Group Cipher \tWIFI_CIPHER_TYPE_CCMP");
        break;
    case WIFI_CIPHER_TYPE_TKIP_CCMP:
        ESP_LOGI(LOG_TAG, "Group Cipher \tWIFI_CIPHER_TYPE_TKIP_CCMP");
        break;
    default:
        ESP_LOGI(LOG_TAG, "Group Cipher \tWIFI_CIPHER_TYPE_UNKNOWN");
        break;
    }
}

// --------------------------------------------------------------------------------------------------------------------
void Wifi::_wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    switch (event_id)
    {
    case WIFI_EVENT_WIFI_READY: /**< ESP32 WiFi ready */
    case WIFI_EVENT_SCAN_DONE:  /**< ESP32 finish scanning AP */
        break;
    case WIFI_EVENT_STA_START: /**< ESP32 station start */
        _started = true;
        last_start_call = 0;
        xEventGroupSetBits(event_group, WIFI_STARTED_BIT);
        ESP_LOGD(LOG_TAG, "WIFI_EVENT_STA_START");
        break;
    case WIFI_EVENT_STA_STOP: /**< ESP32 station stop */
        _started = false;
        _got_ip4 = false;
        xEventGroupSetBits(event_group, WIFI_STOPPED_BIT);
        ESP_LOGD(LOG_TAG, "WIFI_EVENT_STA_STOP");
        break;
    case WIFI_EVENT_STA_CONNECTED: /**< ESP32 station connected to AP */
        _connected = true;
        last_connect_call = 0;
        xEventGroupSetBits(event_group, WIFI_CONNECTED_BIT);
        ESP_LOGD(LOG_TAG, "WIFI_EVENT_STA_CONNECTED");
        break;
    case WIFI_EVENT_STA_DISCONNECTED: /**< ESP32 station disconnected from AP */
        _connected = false;
        _got_ip4 = false;
        xEventGroupSetBits(event_group, WIFI_DISCONNECTED_BIT);
        ESP_LOGD(LOG_TAG, "WIFI_EVENT_STA_DISCONNECTED");
        break;
    case WIFI_EVENT_STA_AUTHMODE_CHANGE: /**< the auth mode of AP connected by ESP32 station changed */

    case WIFI_EVENT_STA_WPS_ER_SUCCESS:     /**< ESP32 station wps succeeds in enrollee mode */
    case WIFI_EVENT_STA_WPS_ER_FAILED:      /**< ESP32 station wps fails in enrollee mode */
    case WIFI_EVENT_STA_WPS_ER_TIMEOUT:     /**< ESP32 station wps timeout in enrollee mode */
    case WIFI_EVENT_STA_WPS_ER_PIN:         /**< ESP32 station wps pin code in enrollee mode */
    case WIFI_EVENT_STA_WPS_ER_PBC_OVERLAP: /**< ESP32 station wps overlap in enrollee mode */

    case WIFI_EVENT_AP_START:           /**< ESP32 soft-AP start */
    case WIFI_EVENT_AP_STOP:            /**< ESP32 soft-AP stop */
    case WIFI_EVENT_AP_STACONNECTED:    /**< a station connected to ESP32 soft-AP */
    case WIFI_EVENT_AP_STADISCONNECTED: /**< a station disconnected from ESP32 soft-AP */
    case WIFI_EVENT_AP_PROBEREQRECVED:  /**< Receive probe request packet in soft-AP interface */
        break;
    default:
        break;
    };
}

// --------------------------------------------------------------------------------------------------------------------
void Wifi::_ip_event_handler(void *arg, esp_event_base_t event_base,
                             int32_t event_id, void *event_data)
{
    //ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
    //ESP_LOGI(LOG_TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));

    switch (event_id)
    {
    case IP_EVENT_STA_GOT_IP: /*!< station got IP from connected AP */
        xEventGroupSetBits(event_group, WIFI_GOT_IP_BIT);
        ESP_LOGI(LOG_TAG, "IP_EVENT_STA_GOT_IP");
        _got_ip4 = true;
        break;
    case IP_EVENT_STA_LOST_IP: /*!< station lost IP and the IP is reset to 0 */
        xEventGroupSetBits(event_group, WIFI_LOST_IP_BIT);
        ESP_LOGI(LOG_TAG, "IP_EVENT_STA_LOST_IP");
        _got_ip4 = false;
        break;
    case IP_EVENT_AP_STAIPASSIGNED: /*!< soft-AP assign an IP to a connected station */
    case IP_EVENT_GOT_IP6:          /*!< station or ap or ethernet interface v6IP addr is preferred */
    case IP_EVENT_ETH_GOT_IP:       /*!< ethernet got IP from connected AP */
    case IP_EVENT_PPP_GOT_IP:       /*!< PPP interface got IP */
    case IP_EVENT_PPP_LOST_IP:      /*!< PPP interface lost IP */

    default:
        break;
    };
}

} // namespace WIFI
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------