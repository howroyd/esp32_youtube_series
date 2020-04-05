#include "MqttClient.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#define LOG_TAG "MQTT"

namespace MQTT
{

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
MqttClientBase* MqttOpenhab::mqtt_base{nullptr};
size_t MqttOpenhab::n_instances = 0;
TaskHandle_t MqttOpenhab::_taskhandle = nullptr;
bool MqttOpenhab::_connected{false};
std::vector<MqttOpenhab::client_event_handler> MqttOpenhab::client_event_handlers;
bool MqttOpenhab::subbed_to_base_topic{false};

// --------------------------------------------------------------------------------------------------------------------
void MqttOpenhab::publish(const std::string &topic, const std::string& data)
{
    mqtt_base->publish(topic, data);
}

// --------------------------------------------------------------------------------------------------------------------
bool MqttOpenhab::subscribe(const std::string &topic, const size_t qos)
{
    return mqtt_base->subscribe(topic, qos);
}

// --------------------------------------------------------------------------------------------------------------------
bool MqttOpenhab::unsubscribe(const std::string &topic)
{
    return mqtt_base->unsubscribe(topic);
}

// --------------------------------------------------------------------------------------------------------------------
void MqttOpenhab::unsubscribe_all(void)
{
    mqtt_base->unsubscribe_all();
}

// --------------------------------------------------------------------------------------------------------------------
void MqttOpenhab::event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    static constexpr const char *MQTT_EVENT = "MQTT_EVENT";
    const esp_mqtt_event_handle_t event = static_cast<esp_mqtt_event_handle_t>(event_data);

    if (mqtt_base->client() == event->client)
    {
        switch (event->event_id)
        {
        case MQTT_EVENT_CONNECTED:
            _connected = true;
            ESP_LOGI(LOG_TAG, "%s_CONNECTED", MQTT_EVENT);
            for (auto& func : client_event_handlers)
                func(MQTT_CONNECTED_BIT);
            
            mqtt_base->subscribe_base();
            break;

        case MQTT_EVENT_DISCONNECTED:
            _connected = false;
            ESP_LOGI(LOG_TAG, "%s_DISCONNECTED", MQTT_EVENT);
            for (auto& func : client_event_handlers)
                func(MQTT_DISCONNECTED_BIT);

            mqtt_base->unsubscribe_all();
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(LOG_TAG, "%s_SUBSCRIBED: msg_id=%d, topic=%.*s", MQTT_EVENT, event->msg_id, event->topic_len, event->topic);
            break;

        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(LOG_TAG, "%s_UNSUBSCRIBED: msg_id=%d, topic=%.*s", MQTT_EVENT, event->msg_id, event->topic_len, event->topic);
            break;

        case MQTT_EVENT_PUBLISHED:
            ESP_LOGD(LOG_TAG, "%s_PUBLISHED: msg_id=%d, topic=%.*s, data=%.*s", MQTT_EVENT, event->msg_id, event->topic_len, event->topic, event->data_len, event->data);
            break;

        case MQTT_EVENT_DATA:
            ESP_LOGI(LOG_TAG, "%s_DATA: %.*s\"%.*s\"", MQTT_EVENT, event->topic_len, event->topic, event->data_len, event->data);
            break;

        case MQTT_EVENT_ERROR:
            ESP_LOGE(LOG_TAG, "%s_ERROR", MQTT_EVENT);
            break;

        case MQTT_EVENT_BEFORE_CONNECT:
            break;

        default:
            ESP_LOGW(LOG_TAG, "%s_%d", MQTT_EVENT, event->event_id);
            break;
        };
    }
}

void MqttOpenhab::_task(void *pvParameters)
{
    while (true)
    {
        if (mqtt_base->initialised())
        {
            if (mqtt_base->wifi_connected())
            {/*
                if (!subbed_to_base_topic)
                {
                    subscribe(mqtt_base->base_topic());
                    subbed_to_base_topic = true;
                }*/
                mqtt_base->publish_my_topic();
                mqtt_base->publish_time();
            }
            else
            {/*
                if (subbed_to_base_topic)
                {
                    unsubscribe(mqtt_base->base_topic());
                    subbed_to_base_topic = false;
                }*/

                mqtt_base->deinit();
            }
        }
        else
        {
            mqtt_base->init();
        }
        vTaskDelay(pdSECOND * 5);
    }
}

// --------------------------------------------------------------------------------------------------------------------
bool MqttOpenhab::connected(void) { return _connected; }

// --------------------------------------------------------------------------------------------------------------------
void MqttOpenhab::register_wifi_event_handler(const client_event_handler event_handler)
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
void MqttOpenhab::deregister_wifi_event_handler(const client_event_handler event_handler)
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
// --------------------------------------------------------------------------------------------------------------------
size_t MqttClientBase::_idx_ctr{0};
MqttClientBase::client_list_t MqttClientBase::_client_list;
char MqttClientBase::mac[13]{};
bool MqttClientBase::_wifi_connected{false};

// --------------------------------------------------------------------------------------------------------------------
bool MqttClientBase::wifi_connected(void) { return _wifi_connected; }

// --------------------------------------------------------------------------------------------------------------------
void MqttClientBase::wifi_event_handler(const EventBits_t bits)
{
    if (bits & WIFI_GOT_IP_BIT)
        _wifi_connected = true;
    else if (bits & WIFI_LOST_IP_BIT)
        _wifi_connected = false;
}

// --------------------------------------------------------------------------------------------------------------------
esp_err_t MqttClientBase::init(const esp_event_handler_t event_handler)
{
    esp_err_t status = ESP_OK;

    if (sem_mqtt_base == nullptr)
    {
        sem_mqtt_base = xSemaphoreCreateBinary();
        xSemaphoreGive(sem_mqtt_base);
    }

    if (xSemaphoreTake(sem_mqtt_base, pdSECOND * 60) == pdTRUE)
    {
        if (!_initialised)
        {
            get_mac();

            while (!wifi_connected())
                vTaskDelay(pdSECOND);

            ESP_LOGD(LOG_TAG, "Initialising MQTT to host \"%s\"", _mqtt_cfg.uri);
            vTaskDelay(pdSECOND);
            _client = esp_mqtt_client_init(&_mqtt_cfg);

            if (event_handler)
                status |= esp_mqtt_client_register_event(_client, static_cast<esp_mqtt_event_id_t>(ESP_EVENT_ANY_ID), event_handler, NULL);

            ESP_LOGD(LOG_TAG, "Starting MQTT to host \"%s\"", _mqtt_cfg.uri);
            vTaskDelay(pdSECOND);
            status |= esp_mqtt_client_start(_client);

            if (status == ESP_OK)
            {
                _initialised = true;
                _client_list.emplace_back(_idx, this);
            }
        }

        xSemaphoreGive(sem_mqtt_base);
    }
    else
    {
        status = ESP_ERR_TIMEOUT;
    }

    return status;
}

// --------------------------------------------------------------------------------------------------------------------
esp_err_t MqttClientBase::deinit(void)
{
    esp_err_t status = ESP_OK;

    if (sem_mqtt_base == nullptr)
    {
        sem_mqtt_base = xSemaphoreCreateBinary();
        xSemaphoreGive(sem_mqtt_base);
    }

    if (xSemaphoreTake(sem_mqtt_base, pdSECOND * 60) == pdTRUE)
    {
        if (_initialised)
        {
            unsubscribe_all();

            esp_mqtt_client_stop(_client);

            esp_mqtt_client_destroy(_client);
            _client = nullptr;

            _initialised = false;

            client_list_t::iterator it = std::find(_client_list.begin(), _client_list.end(), idx_t{_idx, this});

            if (it != _client_list.end())
            {
                _client_list.erase(it);
            }
        }

        xSemaphoreGive(sem_mqtt_base);
    }

    return status;
}

// --------------------------------------------------------------------------------------------------------------------
const char* MqttClientBase::get_mac(void)
{
    uint8_t mac_raw[6];
    esp_efuse_mac_get_default(mac_raw);
    snprintf(mac, sizeof(mac), "%02X%02X%02X%02X%02X%02X",
             mac_raw[0], mac_raw[1], mac_raw[2], mac_raw[3], mac_raw[4], mac_raw[5]);
    return mac;
}

// --------------------------------------------------------------------------------------------------------------------
void MqttClientBase::publish_my_topic(void)
{
    _publish(_client, base_topic(), mac);
}

// --------------------------------------------------------------------------------------------------------------------
void MqttClientBase::publish_time(void)
{
    const std::time_t result{std::time(nullptr)};
    publish("time", std::asctime(std::localtime(&result)));
}

// --------------------------------------------------------------------------------------------------------------------
void MqttClientBase::publish(const std::string &topic, const std::string& data)
{
    _publish(_client, base_topic() + mac + "/" + topic + "/", data);
}

// --------------------------------------------------------------------------------------------------------------------
bool MqttClientBase::subscribe(const std::string &topic, const size_t qos)
{
    const std::string full_topic{base_topic() + topic + "/"};
    ESP_LOGD(LOG_TAG, "Subscribe to \"%s\"", full_topic.c_str());
    return _subscribe(_client, subscriptions, full_topic, qos);
}

// --------------------------------------------------------------------------------------------------------------------
bool MqttClientBase::subscribe_base(const size_t qos)
{
    const std::string device_topic{base_topic() + mac + "/"};

    ESP_LOGI(LOG_TAG, "Base topic \"%s\"", base_topic().c_str());
    ESP_LOGI(LOG_TAG, "Device topic \"%s\"", device_topic.c_str());

    bool success = _subscribe(_client, subscriptions, base_topic(), qos);
    return success & _subscribe(_client, subscriptions, device_topic, qos);
}

// --------------------------------------------------------------------------------------------------------------------
bool MqttClientBase::unsubscribe(const std::string &topic)
{
    return _unsubscribe(_client, subscriptions, base_topic() + topic + "/");
}

// --------------------------------------------------------------------------------------------------------------------
void MqttClientBase::unsubscribe_all(void)
{
    _unsubscribe_all(subscriptions);
}

// --------------------------------------------------------------------------------------------------------------------
void MqttClientBase::_publish(esp_mqtt_client_handle_t client, const std::string &topic, const std::string& data)
{
    ESP_LOGD(LOG_TAG, "Publish to \"%s\" - \"%s\"", topic.c_str(), data.c_str());
    esp_mqtt_client_publish(client, topic.c_str(), data.c_str(), 0, 0, 0);
}

// --------------------------------------------------------------------------------------------------------------------
bool MqttClientBase::_subscribe(esp_mqtt_client_handle_t client, sub_list_t& subscriptions, const std::string &topic, const size_t qos)
{
    bool status = false;

    _unsubscribe(client, subscriptions, topic);

    if (esp_mqtt_client_subscribe(client, topic.c_str(), qos) != -1)
    {
        esp_mqtt_event_t new_sub;
        new_sub.client = client;
        new_sub.session_present = 0;
        new_sub.topic_len = topic.length() + 1;
        new_sub.topic = new char[new_sub.topic_len]{};
        topic.copy(new_sub.topic, sizeof(new_sub.topic_len));

        subscriptions.push_back(new_sub);

        status = true;
    }

    return status;
}

// --------------------------------------------------------------------------------------------------------------------
bool MqttClientBase::_unsubscribe(esp_mqtt_client_handle_t client, sub_list_t& subscriptions, const std::string &topic)
{
    bool status = false;

    auto idx = _find_sub(client, subscriptions, topic);

    if (idx != subscriptions.end())
    {
        if (esp_mqtt_client_unsubscribe(client, idx->topic) != -1)
        {
            delete[] idx->topic;
            subscriptions.erase(idx);
            status = true;
        }
    }

    return status;
}

// --------------------------------------------------------------------------------------------------------------------
void MqttClientBase::_unsubscribe_all(sub_list_t& subscriptions)
{
    for (auto &iter : subscriptions)
    {
        esp_mqtt_client_unsubscribe(iter.client, iter.topic);
        delete[] iter.topic;
    }

    subscriptions.clear();
}

// --------------------------------------------------------------------------------------------------------------------
MqttClientBase::sub_list_t::iterator MqttClientBase::_find_sub(esp_mqtt_client_handle_t client, sub_list_t& subscriptions, const std::string &topic)
{
    std::vector<esp_mqtt_event_t>::iterator iter = subscriptions.end();

    for (auto sub = subscriptions.begin(); sub != subscriptions.end(); ++sub)
    {
        if (client == sub->client)
        {
            // Match found for this client
            if (strcmp(sub->topic, topic.c_str()) == 0)
            {
                // Match found for this topic
                iter = sub;
                break;
            }
        }
    }

    return iter;
}

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

} // namespace MQTT
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------