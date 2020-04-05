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
    } 

    ~Sntp(void)
    {
        sntp_stop();
    }

    static void callback(struct timeval *tv);
    static constexpr const size_t interval_ms{60 * 1000};
public:
    static Sntp& get_instance(void)
    {
        static Sntp sntp;
        return sntp;
    }
};
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

} // namespace SNTP
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------