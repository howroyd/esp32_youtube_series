#pragma once

#include "esp_wifi.h"

#include <mutex>

namespace WIFI
{

class Wifi
{
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
    // YOUTUBE Talk about this, ref above and CPP Weekly
    Wifi(void);
    ~Wifi(void)                     = default;
    Wifi(const Wifi&)               = default;
    Wifi(Wifi&&)                    = default;
    Wifi& operator=(const Wifi&)    = default;
    Wifi& operator=(Wifi&&)         = default;

    esp_err_t init(void);       // TODO Set everything up
    esp_err_t begin(void);      // TODO Start WiFi, connect, etc

    state_e get_state(void);    // TODO

    // YOUTUBE talk about this being constexpr
    constexpr static const char* get_mac(void) 
        { return mac_addr_cstr; }

private:
    void state_machine(void);   // TODO

    // Get the MAC from the API and convert to ASCII HEX
    // YOUTUBE Why is this static
    static esp_err_t _get_mac(void);

    static char mac_addr_cstr[13];  ///< Buffer to hold MAC as cstring
    static std::mutex init_mutx;    ///< Initialisation mutex
};


} // namespace WIFI