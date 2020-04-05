// REF: https://github.com/espressif/esp-idf/blob/600d542f53ab6540d277fca4716824d168244c8c/examples/common_components/protocol_examples_common/connect.c

#pragma once

#include "include/MqttClient_Includes.h"

#define HOST "mqtt://96.69.1.254" // Insert MQTT server address here

namespace MQTT
{

// --------------------------------------------------------------------------------------------------------------------
class MqttClientBase : private WIFI::Wifi
{
    static constexpr esp_event_loop_args_t _loop_args = {.queue_size = 16};
    static size_t _idx_ctr;
    size_t _idx{};

    typedef std::pair<size_t, MqttClientBase *> idx_t;
    typedef std::vector<idx_t> client_list_t;
    static client_list_t _client_list;

    static char mac[13];

    SNTP::Sntp &_sntp;

public:
    typedef std::vector<esp_mqtt_event_t> sub_list_t;

    MqttClientBase(const std::string& host, const std::string& base_topic, const esp_event_handler_t event_handler = nullptr) : 
        _idx{_idx_ctr++},
        _sntp{SNTP::Sntp::get_instance()},
        _host{std::string{"mqtt://"} + host},
        _base_topic{std::string{"/"} + base_topic + std::string{"/"}}
    {
        register_wifi_event_handler(&wifi_event_handler);

        esp_event_loop_create(&_loop_args, &_event_loop);

        _mqtt_cfg.event_loop_handle = _event_loop;
        _mqtt_cfg.uri = _host.c_str();
        _mqtt_cfg.user_context = &_idx;

        init(event_handler);
    }

    ~MqttClientBase(void)
    {
        deinit();

        deregister_wifi_event_handler(&wifi_event_handler);

        vSemaphoreDelete(sem_mqtt_base);
        sem_mqtt_base = nullptr;

        esp_event_loop_delete(_event_loop);
        _event_loop = nullptr;
    }

    void publish_my_topic(void);
    void publish_time(void);

    void publish(const std::string &topic, const std::string& data);
    bool subscribe(const std::string &topic, const size_t qos = 1);
    bool subscribe_base(const size_t qos = 1);
    bool unsubscribe(const std::string &topic);
    void unsubscribe_all(void);

    static bool wifi_connected(void);
    esp_mqtt_client_config_t config(void) const { return _mqtt_cfg; }
    esp_mqtt_client_handle_t client(void) const { return _client; }
    std::string base_topic(void) const { return _base_topic; }
    bool initialised(void) const { return _initialised; }

//protected:
    esp_err_t init(const esp_event_handler_t event_handler = nullptr);
    esp_err_t deinit(void);

private:
    static void wifi_event_handler(const EventBits_t bits);

    static void _publish(esp_mqtt_client_handle_t client, const std::string &topic, const std::string& data);

    static sub_list_t::iterator _find_sub(esp_mqtt_client_handle_t client, sub_list_t& subscriptions, const std::string &topic);
    static bool _subscribe(esp_mqtt_client_handle_t client, sub_list_t& subscriptions, const std::string &topic, const size_t qos = 1);
    static bool _unsubscribe(esp_mqtt_client_handle_t client, sub_list_t& subscriptions, const std::string &topic);
    static void _unsubscribe_all(sub_list_t& subscriptions);

    static const char* get_mac(void);

    bool _initialised{false};
    const std::string _host;
    const std::string _base_topic;
    sub_list_t subscriptions;
    SemaphoreHandle_t sem_mqtt_base{};
    static bool _wifi_connected;

    esp_mqtt_client_config_t _mqtt_cfg{};
    esp_mqtt_client_handle_t _client{};
    esp_event_loop_handle_t _event_loop{};
};

#define MQTT_CONNECTED_BIT BIT0
#define MQTT_DISCONNECTED_BIT BIT1
#define MQTT_ALL_BITS (MQTT_CONNECTED_BIT | MQTT_DISCONNECTED_BIT)

// --------------------------------------------------------------------------------------------------------------------
class MqttOpenhab
{
    static constexpr const char* ip{"96.69.1.254"};

public:
    typedef void (*client_event_handler)(const EventBits_t bits);

    MqttOpenhab(void)
    {
        if (n_instances == 0)
        {
            if(!mqtt_base)
                mqtt_base = new MqttClientBase{ip, "esp32", &event_handler};

            xTaskCreate(_task, "MqttOpenhab", 1024 * 4, nullptr, TaskPrio_Low, &_taskhandle);
        }

        ++n_instances;
    }

    ~MqttOpenhab(void)
    {
        --n_instances;

        if (n_instances == 0)
        {
            if (mqtt_base)
            {
                mqtt_base->deinit();
                delete mqtt_base;
                mqtt_base = nullptr;
            }

            client_event_handlers.clear();

            if (_taskhandle)
            {
                vTaskDelete(_taskhandle);
                _taskhandle = nullptr;
            }
        }
    }

    static bool connected(void);

    static void register_wifi_event_handler(const client_event_handler event_handler);
    static void deregister_wifi_event_handler(const client_event_handler event_handler);

    static void publish(const std::string &topic, const std::string& data);
    static bool subscribe(const std::string &topic, const size_t qos = 1);
    static bool unsubscribe(const std::string &topic);
    static void unsubscribe_all(void);

private:
    static MqttClientBase* mqtt_base;

    static void event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data);
    static std::vector<client_event_handler> client_event_handlers;

    static bool _connected;
    static bool subbed_to_base_topic;

    static void _task(void *pvParameters);
    static TaskHandle_t _taskhandle;

    static size_t n_instances;
};

// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------


} // namespace MQTT
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------