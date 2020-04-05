#include "SntpTime.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#define LOG_TAG "SNTP"

namespace SNTP
{
    
void Sntp::callback(struct timeval *tv)
{
    const std::time_t result{std::time(nullptr)};
    const char* asc_time{std::asctime(std::localtime(&result))};
    ESP_LOGD(LOG_TAG, "Time is %s", asc_time);
}

} // namespace SNTP
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------