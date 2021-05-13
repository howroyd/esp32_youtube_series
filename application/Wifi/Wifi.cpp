#include "Wifi.h"

namespace WIFI
{

// Wifi statics
char                Wifi::mac_addr_cstr[]{};    ///< Buffer to hold MAC as cstring
std::mutex          Wifi::init_mutx{};          ///< Initialisation mutex
std::mutex          Wifi::connect_mutx{};       ///< Connect mutex
std::mutex          Wifi::state_mutx{};         ///< State change mutex
Wifi::state_e       Wifi::_state{state_e::NOT_INITIALISED};
wifi_init_config_t  Wifi::wifi_init_config = WIFI_INIT_CONFIG_DEFAULT();
wifi_config_t       Wifi::wifi_config{};
NVS::Nvs            Wifi::nvs{};

Wifi::smartconfig_state_e   Wifi::smartconfig_state{smartconfig_state_e::NOT_STARTED};
smartconfig_start_config_t  Wifi::smartconfig_config{true, false, nullptr};

// Wifi Constructor
Wifi::Wifi(void)
{
    // Aquire our initialisation mutex to ensure only one
    //   thread (multi-cpu safe) is running this
    //   constructor at once.  No running twice in parallel!
    std::lock_guard<std::mutex> guard(init_mutx);

    // Check if the MAC cstring currently begins with a
    //   nullptr, i.e. is default initialised, not set
    if (!get_mac()[0])
    {
        // Get the MAC and if this fails restart
        if (ESP_OK != _get_mac())
            esp_restart();
    }
}

void Wifi::event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (WIFI_EVENT == event_base)
    {
        ESP_LOGI(_log_tag, "%s:%d Got a WIFI_EVENT", __func__, __LINE__);
        return wifi_event_handler(arg, event_base, event_id, event_data);
    }
    else if (IP_EVENT == event_base)
    {
        ESP_LOGI(_log_tag, "%s:%d Got an IP_EVENT", __func__, __LINE__);
        return ip_event_handler(arg, event_base, event_id, event_data);
    }
    else
    {
        ESP_LOGE(_log_tag, "%s:%d Unexpected event: %s", __func__, __LINE__, event_base);
    }
}

void Wifi::wifi_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (WIFI_EVENT == event_base)
    {
        const wifi_event_t event_type{static_cast<wifi_event_t>(event_id)};

        ESP_LOGI(_log_tag, "%s:%d Event ID %d", __func__, __LINE__, event_id);

        switch(event_type)
        {
        case WIFI_EVENT_STA_START:
        {
            ESP_LOGI(_log_tag, "%s:%d STA_START, waiting for state_mutx", __func__, __LINE__);
            std::lock_guard<std::mutex> state_guard(state_mutx); // TODO waiting for mutx in callback
            _state = state_e::READY_TO_CONNECT;
            ESP_LOGI(_log_tag, "%s:%d READY_TO_CONNECT", __func__, __LINE__);
            break;
        }

        case WIFI_EVENT_STA_CONNECTED:
        {
            ESP_LOGI(_log_tag, "%s:%d STA_CONNECTED, waiting for state_mutx", __func__, __LINE__);
            std::lock_guard<std::mutex> state_guard(state_mutx);
            ESP_LOGI(_log_tag, "%s:%d WAITING_FOR_IP", __func__, __LINE__);
            _state = state_e::WAITING_FOR_IP;
            break;
        }

        default:
            // TODO STOP and DISCONNECTED, and others
            ESP_LOGW(_log_tag, "%s:%d Default switch case (%d)", __func__, __LINE__, event_id);
            break;
        }
    }
}

void Wifi::ip_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (IP_EVENT == event_base)
    {
        const ip_event_t event_type{static_cast<ip_event_t>(event_id)};

        ESP_LOGI(_log_tag, "%s:%d Event ID %d", __func__, __LINE__, event_id);

        switch(event_type)
        {
        case IP_EVENT_STA_GOT_IP:
        {
            ESP_LOGI(_log_tag, "%s:%d Got IP, waiting for state_mutx", __func__, __LINE__);
            std::lock_guard<std::mutex> guard(state_mutx);
            _state = state_e::CONNECTED;
            ESP_LOGI(_log_tag, "%s:%d CONNECTED!", __func__, __LINE__);
            break;
        }

        case IP_EVENT_STA_LOST_IP:
        {
            ESP_LOGW(_log_tag, "%s:%d Lost IP, waiting for state_mutx", __func__, __LINE__);
            std::lock_guard<std::mutex> guard(state_mutx);
            _state = state_e::WAITING_FOR_IP;
            ESP_LOGI(_log_tag, "%s:%d WAITING_FOR_IP", __func__, __LINE__);
            break;
        }

        default:
            // TODO IP6
            ESP_LOGW(_log_tag, "%s:%d Default switch case (%d)", __func__, __LINE__, event_id);
            break;
        }
    }
}

void Wifi::sc_event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (SC_EVENT == event_base)
    {
        const smartconfig_event_t event_type{static_cast<smartconfig_event_t>(event_id)};

        ESP_LOGI(_log_tag, "%s:%d Event ID %d", __func__, __LINE__, event_id);

        switch(event_type)
        {
        case SC_EVENT_GOT_SSID_PSWD:
        {
            const smartconfig_event_got_ssid_pswd_t* const
                data{static_cast<smartconfig_event_got_ssid_pswd_t*>(event_data)};

            if (_state == state_e::READY_TO_CONNECT)
            {
                const esp_err_t sc_ack_status = sc_send_ack_start(data->type, data->token, data->cellphone_ip);
                if (sc_ack_status != ESP_OK) 
                {
                    ESP_LOGE(LOG_TAG, "Send smartconfig ACK error: %s", esp_err_to_name(sc_ack_status));
                }

                memcpy(wifi_config.sta.ssid, data->ssid,
                        std::min(sizeof(wifi_sta_config_t::ssid),
                                    sizeof(smartconfig_event_got_ssid_pswd_t::ssid)));

                memcpy(wifi_config.sta.password, data->password,
                        std::min(sizeof(wwifi_sta_config_t::password), 
                                    sizeof(smartconfig_event_got_ssid_pswd_t::password)));

                ESP_LOGI(_log_tag, "%s:%d Calling esp_wifi_set_config", __func__, __LINE__);
                esp_err_t status = esp_wifi_set_config(WIFI_IF_STA, &wifi_config); // TODO keep track of mode
                ESP_LOGI(_log_tag, "%s:%d esp_wifi_set_config:%s", __func__, __LINE__,
                                                            esp_err_to_name(status));

                if (ESP_OK == status)
                {
                    ESP_LOGI(_log_tag, "%s:%d Waiting for connect_mutx", __func__, __LINE__);
                    std::lock_guard<std::mutex> connect_guard(connect_mutx);

                    ESP_LOGI(_log_tag, "%s:%d Calling esp_wifi_connect", __func__, __LINE__);
                    status = esp_wifi_connect();
                    ESP_LOGI(_log_tag, "%s:%d esp_wifi_connect:%s", __func__, __LINE__,
                                                                    esp_err_to_name(status));
                }

                if (ESP_OK == status)
                {
                    ESP_LOGI(_log_tag, "%s:%d Waiting for state_mutx", __func__, __LINE__);
                    std::lock_guard<std::mutex> state_guard(state_mutx);
                    _state = state_e::CONNECTING;
                }
            }
            break;
        }
        default: // Don't have any work to do for the other event types
            break;
        }
    }
}

esp_err_t Wifi::init(void)
{
    return _init();
}


/*
const size_t ssid_len_to_copy       = std::min(strlen(ssid), 
                                sizeof(wifi_config.sta.ssid));

memcpy(wifi_config.sta.ssid, ssid, ssid_len_to_copy);

const size_t password_len_to_copy   = std::min(strlen(password),
                                        sizeof(wifi_config.sta.password));

memcpy(wifi_config.sta.password, password, password_len_to_copy);
*/

esp_err_t Wifi::begin(void)
{
    ESP_LOGI(_log_tag, "%s:%d Waiting for connect_mutx", __func__, __LINE__);
    std::lock_guard<std::mutex> connect_guard(connect_mutx);

    esp_err_t status{ESP_OK};

    ESP_LOGI(_log_tag, "%s:%d Waiting for state_mutx", __func__, __LINE__);
    std::lock_guard<std::mutex> state_guard(state_mutx);
    switch(_state)
    {
    case state_e::READY_TO_CONNECT:
        if (!empty_credentials())
        {
            ESP_LOGI(_log_tag, "%s:%d Calling esp_wifi_connect", __func__, __LINE__);
            status = esp_wifi_connect();
            ESP_LOGI(_log_tag, "%s:%d esp_wifi_connect:%s", __func__, __LINE__,
                                                            esp_err_to_name(status));

            if (ESP_OK == status)
                _state = state_e::CONNECTING;
        }
        else if (smartconfig_state_e::NOT_STARTED == smartconfig_state)
        {
            ESP_LOGI(_log_tag, "%s:%d Calling esp_smartconfig_start", __func__, __LINE__);
            status = esp_smartconfig_start(&smartconfig_config);
            ESP_LOGI(_log_tag, "%s:%d esp_smartconfig_start:%s", __func__, __LINE__,
                                                                esp_err_to_name(status));

            if (ESP_OK == status)
                smartconfig_state = smartconfig_state_e::STARTED;
        }
        break;
    case state_e::CONNECTING:
    case state_e::WAITING_FOR_IP:
    case state_e::CONNECTED:
        break;
    case state_e::NOT_INITIALISED:
    case state_e::INITIALISED:
    case state_e::DISCONNECTED:
    case state_e::ERROR:
        ESP_LOGE(_log_tag, "%s:%d Error state", __func__, __LINE__);
        status = ESP_FAIL;
        break;
    }

    return status;
}

esp_err_t Wifi::_init(void)
{
    ESP_LOGI(_log_tag, "%s:%d Waiting for init_mutx", __func__, __LINE__);
    std::lock_guard<std::mutex> init_guard(init_mutx);

    esp_err_t status{ESP_OK};

    ESP_LOGI(_log_tag, "%s:%d Waiting for state_mutx", __func__, __LINE__);
    std::lock_guard<std::mutex> state_guard(state_mutx);

    if (state_e::NOT_INITIALISED == _state)
    {
        ESP_LOGI(_log_tag, "%s:%d Calling esp_netif_init", __func__, __LINE__);
        status = esp_netif_init();
        ESP_LOGI(_log_tag, "%s:%d esp_netif_init:%s", __func__, __LINE__,
                                                        esp_err_to_name(status));

        if (ESP_OK == status)
        {
            ESP_LOGI(_log_tag, "%s:%d Calling esp_netif_create_default_wifi_sta", __func__, __LINE__);
            const esp_netif_t* const p_netif = esp_netif_create_default_wifi_sta();
            ESP_LOGI(_log_tag, "%s:%d esp_netif_create_default_wifi_sta:%p", __func__, __LINE__,
                                                        p_netif);

            if (!p_netif) status = ESP_FAIL;
        }

        if (ESP_OK == status)
        {
            ESP_LOGI(_log_tag, "%s:%d Calling esp_wifi_init", __func__, __LINE__);
            status = esp_wifi_init(&wifi_init_config);
            ESP_LOGI(_log_tag, "%s:%d esp_wifi_init:%s", __func__, __LINE__,
                                                        esp_err_to_name(status));
        }

        if (ESP_OK == status)
        {
            ESP_LOGI(_log_tag, "%s:%d Calling esp_event_handler_instance_register", __func__, __LINE__);
            status = esp_event_handler_instance_register(WIFI_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &wifi_event_handler,
                                                            nullptr,
                                                            nullptr);
            ESP_LOGI(_log_tag, "%s:%d esp_event_handler_instance_register:%s", __func__, __LINE__,
                                                        esp_err_to_name(status));
        }

        if (ESP_OK == status)
        {
            ESP_LOGI(_log_tag, "%s:%d Calling esp_event_handler_instance_register", __func__, __LINE__);
            status = esp_event_handler_instance_register(IP_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &ip_event_handler,
                                                            nullptr,
                                                            nullptr);
            ESP_LOGI(_log_tag, "%s:%d esp_event_handler_instance_register:%s", __func__, __LINE__,
                                                        esp_err_to_name(status));
        }

        if (ESP_OK == status)
        {
            ESP_LOGI(_log_tag, "%s:%d Calling esp_event_handler_instance_register", __func__, __LINE__);
            status = esp_event_handler_instance_register(SC_EVENT,
                                                            ESP_EVENT_ANY_ID,
                                                            &sc_event_handler,
                                                            nullptr,
                                                            nullptr);
            ESP_LOGI(_log_tag, "%s:%d esp_event_handler_instance_register:%s", __func__, __LINE__,
                                                        esp_err_to_name(status));
        }

        if (ESP_OK == status)
        {
            ESP_LOGI(_log_tag, "%s:%d Calling esp_wifi_set_mode", __func__, __LINE__);
            status = esp_wifi_set_mode(WIFI_MODE_STA); // TODO keep track of mode
            ESP_LOGI(_log_tag, "%s:%d esp_wifi_set_mode:%s", __func__, __LINE__,
                                                        esp_err_to_name(status));
        }
        
        if (ESP_OK == status)
        {
            wifi_config.sta.threshold.authmode  = WIFI_AUTH_WPA2_PSK;
            wifi_config.sta.pmf_cfg.capable     = true;
            wifi_config.sta.pmf_cfg.required    = false;

            ESP_LOGI(_log_tag, "%s:%d Calling esp_wifi_set_config", __func__, __LINE__);
            status = esp_wifi_set_config(WIFI_IF_STA, &wifi_config); // TODO keep track of mode
            ESP_LOGI(_log_tag, "%s:%d esp_wifi_set_config:%s", __func__, __LINE__,
                                                        esp_err_to_name(status));
        }
        
        if (ESP_OK == status)
        {
            ESP_LOGI(_log_tag, "%s:%d Calling esp_wifi_start", __func__, __LINE__);
            status = esp_wifi_start();
            ESP_LOGI(_log_tag, "%s:%d esp_wifi_start:%s", __func__, __LINE__,
                                                        esp_err_to_name(status));
        }

        if (ESP_OK == status)
        {
            ESP_LOGI(_log_tag, "%s:%d INITIALISED", __func__, __LINE__);
            _state = state_e::INITIALISED;
        }
    }
    else if (state_e::ERROR == _state)
    {
        ESP_LOGE(_log_tag, "%s:%d FAILED", __func__, __LINE__);
        status = ESP_FAIL;
    }

    return status;
}

// Get the MAC from the API and convert to ASCII HEX
esp_err_t Wifi::_get_mac(void)
{
    uint8_t mac_byte_buffer[6]{};   ///< Buffer to hold MAC as bytes

    // Get the MAC as bytes from the ESP API
    const esp_err_t 
        status{esp_efuse_mac_get_default(mac_byte_buffer)};

    if (ESP_OK == status)
    {
        // Convert the bytes to a cstring and store
        //   in our static buffer as ASCII HEX
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