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

class Sntp final : private WIFI::Wifi
{
    constexpr static const char* _log_tag{"Sntp"};

    Sntp(void) = default;
    ~Sntp(void) { sntp_stop(); }

    static void callback_on_ntp_update(timeval* tv);

public:
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
    };

    static esp_err_t init(void);

    [[nodiscard]] static const auto time_point_now(void) noexcept
        { return std::chrono::system_clock::now(); }

    [[nodiscard]] static const auto time_since_last_update(void) noexcept
        { return time_point_now() - last_update; }

    [[nodiscard]] static const char* ascii_time_now(void);

    [[nodiscard]] static std::chrono::seconds epoch_seconds(void) noexcept
    {
        return std::chrono::duration_cast<std::chrono::seconds> \
                                    (time_point_now().time_since_epoch());
    }

private:
    static std::chrono::_V2::system_clock::time_point last_update;
    static time_source_e source;
    static bool _running;
};

} // namespace SNTP