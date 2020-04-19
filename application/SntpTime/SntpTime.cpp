#include "SntpTime.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#define LOG_TAG "SNTP"

namespace SNTP
{

std::time_t Sntp::last_update{};
Sntp::time_source_e Sntp::source{Sntp::time_source_e::TIME_SRC_UNKNOWN};

std::tm Sntp::time_since_last_update(void)
{
  std::tm ret{};

  const std::time_t now{std::time(nullptr)};

  const uint32_t diff{static_cast<uint32_t>(difftime(now, last_update))};

  ret.tm_yday = diff >= day_to_sec ? (diff - (diff % day_to_sec)) / day_to_sec : 0;
  ret.tm_hour = diff >= hour_to_sec ? (diff - (diff % hour_to_sec)) / hour_to_sec : 0;
  ret.tm_sec = diff % 60;

  return ret;
}

void Sntp::callback(struct timeval *tv)
{
    last_update = std::time(nullptr);
    const char* asc_time{std::asctime(std::localtime(&last_update))};
    ESP_LOGD(LOG_TAG, "Time is %s", asc_time);
}

} // namespace SNTP
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------
  // --------------------------------------------------------------------------------------------------------------------