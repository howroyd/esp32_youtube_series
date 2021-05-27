#include <ctime>
#include <chrono>
#include <iomanip>
#include <string>

#include "esp_sntp.h"

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "Wifi.h"

namespace SNTP
{

/// @brief Simple Network Time Protocol
///
/// Uses the ESP-IDF API to obtain real world time from internet NTP servers
///
/// Synchronises to system time; works with std::chrono and <ctime>
///
/// @attention Final so not inheritable
/// @attention Intended to be a singleton object
/// TODO Make this copyable and threadsafe
class Sntp final : private WIFI::Wifi
{
    constexpr static const char* _log_tag{"Sntp"};

    Sntp(void) = default;           ///< Default constructable
    ~Sntp(void) { sntp_stop(); }    ///< Destructor which stops the SNTP API

	/// @brief Callback for when a new SNTP update happens
	///
    /// @param[in] tv : ctime timeval struct pointer
    static void callback_on_ntp_update(timeval* tv);

public:
	/// @brief Get a reference to the singleton instance
	///
	/// @return reference to a singleton SNTP object
    static Sntp& get_instance(void)
    {
        static Sntp sntp;
        return sntp;
    }

    enum time_source_e : uint8_t
    {
        TIME_SRC_UNKNOWN    = 0,
        TIME_SRC_NTP        = 1,
        TIME_SRC_GPS        = 2,
        TIME_SRC_RADIO      = 3,
        TIME_SRC_MANUAL     = 4,
        TIME_SRC_ATOMIC_CLK = 5,
        TIME_SRC_CELL_NET   = 6
    }; ///< SNTP API time source

	/// @brief Initialise and start the SNTP service (blocking)
	///
    /// @attention Will block until WiFi is connected
    ///
	/// @return 
	/// 	- ESP_OK if SNTP running, else ESP_FAIL
    static esp_err_t init(void);

    [[nodiscard]] static const auto time_point_now(void) noexcept
        { return std::chrono::system_clock::now(); }        ///< Return the current time

    [[nodiscard]] static const auto time_since_last_update(void) noexcept
        { return time_point_now() - last_update; }          ///< Return how long since the last NTP update

    [[nodiscard]] static const char* ascii_time_now(void);  ///< Return the current time as a cstring

    [[nodiscard]] static std::chrono::seconds epoch_seconds(void) noexcept
    {
        return std::chrono::duration_cast<std::chrono::seconds> \
                                    (time_point_now().time_since_epoch());
    } ///< Return the current time in seconds since epoch

private:
    static std::chrono::_V2::system_clock::time_point last_update; ///< Time of the last NTP update
    static time_source_e source;    ///< SNTP API time source
    static bool _running;           ///< SNTP running flag
};

} // namespace SNTP