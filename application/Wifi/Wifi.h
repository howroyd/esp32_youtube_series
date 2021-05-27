#pragma once

#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_smartconfig.h"
#include "smartconfig_ack.h"

#include <algorithm>
#include <mutex>

#include <cstring>

#include "../Nvs32/Nvs32.h"

namespace WIFI
{

/// @brief Non-volatile Storage Partition Interface
///
/// @note Threadsafe
class Wifi
{
    constexpr static const char* _log_tag{"WiFi"};          ///< cstring of logging tag

    constexpr static const char* ssid{"GreenGiant-TEST"};   ///< cstring of hard coded WiFi SSID
    constexpr static const char* password{"GreenGiant"};    ///< cstring of hard coded WiFi password

public:
    enum class state_e
    {
        NOT_INITIALISED,
        INITIALISED,
        READY_TO_CONNECT,
        CONNECTING,
        WAITING_FOR_IP,
        CONNECTED,
        DISCONNECTED,
        ERROR
    }; ///< WiFi states

    // "Rule of 5" Constructors and assignment operators
    // Ref: https://en.cppreference.com/w/cpp/language/rule_of_three

    /// @brief WiFi Instance Constructor
    Wifi(void);
    ~Wifi(void)                     = default;
    Wifi(const Wifi&)               = default;
    Wifi(Wifi&&)                    = default;
    Wifi& operator=(const Wifi&)    = default;
    Wifi& operator=(Wifi&&)         = default;

	/// @brief Initialise WiFi
	///
    /// @note Non-blocking
    ///
	/// @return 
	/// 	- ESP_OK if WIFI driver initialised
    ///     - ESP_FAIL if we are in the ERROR state (//TODO how to recover?)
    ///     - other error codes from underlying API
    esp_err_t init(void);

	/// @brief Start WiFi and connect to AP (non-blocking)
	///
    /// @note Implicitly calls init if required
    /// @note Non-blocking
    ///
	/// @return 
	/// 	- ESP_OK if WIFI driver running
    ///     - ESP_FAIL if we are in the ERROR state (//TODO how to recover?)
    ///     - other error codes from underlying API
    esp_err_t begin(void);

    constexpr static const state_e& get_state(void) { return _state; } ///< Current WiFi state

    constexpr static const char* get_mac(void) 
        { return mac_addr_cstr; } ///< Device specific WiFi MAC address

private:
    enum class smartconfig_state_e
    {
        NOT_STARTED,
        STARTED
    }; ///< SmartConfig states

    static smartconfig_state_e  smartconfig_state; ///< Current SmartConfig state

	/// @brief Initialise WiFi
	///
    /// @note Non-blocking
    ///
	/// @return 
	/// 	- ESP_OK if WIFI driver initialised
    ///     - ESP_FAIL if we are in the ERROR state (//TODO how to recover?)
    ///     - other error codes from underlying API
    static esp_err_t                    _init(void);

    static wifi_init_config_t           wifi_init_config;   ///< WiFi init config
    static wifi_config_t                wifi_config;        ///< WiFi config containing SSID & password
    static smartconfig_start_config_t   smartconfig_config; ///< SmartConfig config

	/// @brief Check if current credentials are not empty cstrings
    ///
	/// @return 
	/// 	- true if SSID and password are not null, else false
    static bool empty_credentials(void)
    {
        if ('\0' == wifi_config.sta.ssid[0] ||
            '\0' == wifi_config.sta.password[0])
            return true;
        return false;
    }

    void state_machine(void);   // TODO static

    static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data);
    static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data);
    static void ip_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data);
    static void sc_event_handler(void* arg, esp_event_base_t event_base,
                                    int32_t event_id, void* event_data); ///< smart config

    static state_e _state;

    /// @brief Get the MAC from the API and convert to ASCII HEX
    ///
	/// @return 
	/// 	- ESP_OK if MAC obtained
    ///     - other error codes from underlying API
    static esp_err_t _get_mac(void);

    static char mac_addr_cstr[13];  ///< Buffer to hold MAC as cstring
    static std::mutex init_mutx;    ///< Initialisation mutex
    static std::mutex connect_mutx; ///< Connect mutex
    static std::mutex state_mutx;   ///< State change mutex

    static NVS::Nvs nvs;            ///< NVS instance for saving SSID and password
};


} // namespace WIFI