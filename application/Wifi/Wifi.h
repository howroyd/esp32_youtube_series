#pragma once

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"

#include <algorithm>
#include <mutex>

#include <cstring>

namespace WIFI
{

class Wifi
{
    constexpr static const char* ssid{"MyWifiSsid"};
    constexpr static const char* password{"MyWifiPassword"};

public:
    // Strongly typed enum to define our state
    enum class state_e
    {
        NOT_INITIALISED,
        INITIALISED,
        WAITING_FOR_CREDENTIALS,
        READY_TO_CONNECT,
        CONNECTING,
        WAITING_FOR_IP,
        CONNECTED,
        DISCONNECTED,
        ERROR
    };

    // "Rule of 5" Constructors and assignment operators
    // Ref: https://en.cppreference.com/w/cpp/language/rule_of_three
    Wifi(void);
    ~Wifi(void)                     = default;
    Wifi(const Wifi&)               = default;
    Wifi(Wifi&&)                    = default;
    Wifi& operator=(const Wifi&)    = default;
    Wifi& operator=(Wifi&&)         = default;

    esp_err_t init(void);       // TODO Set everything up
    esp_err_t begin(void);      // TODO Start WiFi, connect, etc

    state_e get_state(void);    // TODO constexpr static const state_e&

    constexpr static const char* get_mac(void) 
        { return mac_addr_cstr; }

private:
    static esp_err_t _init(void);
    static wifi_init_config_t wifi_init_config;
    static wifi_config_t wifi_config;

    void state_machine(void);   // TODO static
    static state_e _state;             // TODO static

    // Get the MAC from the API and convert to ASCII HEX
    static esp_err_t _get_mac(void);

    static char mac_addr_cstr[13];  ///< Buffer to hold MAC as cstring
    static std::mutex init_mutx;    ///< Initialisation mutex
};


} // namespace WIFI