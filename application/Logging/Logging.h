#pragma once

#include "esp_err.h"
#include "esp_log.h"

#include <chrono>
#include <mutex>
#include <experimental/source_location>

#include <cstring>

namespace LOGGING
{

class Logging
{
    using Ms = std::chrono::milliseconds;

public:
    static esp_err_t log(const esp_log_level_t level,
                            const std::string_view& msg,
                            const std::experimental::source_location& location = std::experimental::source_location::current())
    {
        if (mutx.try_lock_for(Ms(100)))
        {
            ESP_LOG_LEVEL(level, location.file_name(), "[%d:%s]: %.*s", location.line(), location.function_name(), msg.length(), msg.data());
            mutx.unlock();
            return ESP_OK;
        }
        return ESP_ERR_TIMEOUT;
    }

private:
    static std::recursive_timed_mutex mutx;
};

} // namespace LOGGING