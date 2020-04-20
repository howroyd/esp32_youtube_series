#include <ctime>

#include "esp_sntp.h"

#pragma once

#include "TaskCpp.h"
#include "Wifi.h"

namespace SNTP
{

// --------------------------------------------------------------------------------------------------------------------
class Sntp final : private WIFI::Wifi
{
    Sntp(void)
    {
        setenv("TZ", "GMT0BST,M3.5.0/1,M10.5.0", 1); // London
        //setenv("TZ", "GMTGMT-1,M3.4.0/01,M10.4.0/02", 1); // DST
        //setenv("TZ", "GMT+0BST-1,M3.5.0/01,M10.5.0/02", 1); // DST
        sntp_setoperatingmode(SNTP_OPMODE_POLL);

        sntp_setservername(0, "time.google.com");
        sntp_setservername(1, "pool.ntp.org");
        
        sntp_set_time_sync_notification_cb(&callback);
        sntp_set_sync_interval(interval_ms);
        sntp_init();

        source = TIME_SRC_NTP;
    } 

    ~Sntp(void)
    {
        sntp_stop();
    }

    static void callback(struct timeval *tv);
    static constexpr const size_t interval_ms{60 * 1000};

    static std::time_t last_update;
public:
    static Sntp& get_instance(void)
    {
        static Sntp sntp;
        return sntp;
    }

    static std::tm time_since_last_update(void);

    static constexpr const uint32_t min_to_sec{60};
    static constexpr const uint32_t hour_to_sec{min_to_sec * 60};
    static constexpr const uint32_t day_to_sec{hour_to_sec * 24};

    enum time_source_e : uint8_t
    {
        TIME_SRC_UNKNOWN = 0,
        TIME_SRC_NTP = 1,
        TIME_SRC_GPS = 2,
        TIME_SRC_RADIO = 3,
        TIME_SRC_MANUAL = 4,
        TIME_SRC_ATOMIC_CLK = 5,
        TIME_SRC_CELL_NET = 6,
    };

    static time_source_e source;
};
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

} // namespace SNTP
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------