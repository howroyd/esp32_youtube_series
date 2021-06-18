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

    esp_err_t operator() (const esp_log_level_t level,
                            const std::string_view& msg,
                            const std::experimental::source_location& location = std::experimental::source_location::current())
    {
        return log(level, msg, location);
    }

    static esp_err_t error(const std::string_view& msg,
                            const std::experimental::source_location& location = std::experimental::source_location::current())
    {
        return log(ESP_LOG_ERROR, msg, location);
    }

    static esp_err_t warning(const std::string_view& msg,
                            const std::experimental::source_location& location = std::experimental::source_location::current())
    {
        return log(ESP_LOG_WARN, msg, location);
    }

    static esp_err_t info(const std::string_view& msg,
                            const std::experimental::source_location& location = std::experimental::source_location::current())
    {
        return log(ESP_LOG_INFO, msg, location);
    }

    static esp_err_t debug(const std::string_view& msg,
                            const std::experimental::source_location& location = std::experimental::source_location::current())
    {
        return log(ESP_LOG_DEBUG, msg, location);
    }

    static esp_err_t verbose(const std::string_view& msg,
                            const std::experimental::source_location& location = std::experimental::source_location::current())
    {
        return log(ESP_LOG_VERBOSE, msg, location);
    }

    template<class Rep, class Period>
    [[nodiscard]] static esp_err_t lock(std::unique_lock<std::recursive_timed_mutex>& lock, const std::chrono::duration<Rep, Period>& timeout_duration = Ms(100))
    {
        if (mutx.try_lock_for(timeout_duration))
        {
            lock = std::move(std::unique_lock<std::recursive_timed_mutex>(mutx));
            mutx.unlock(); // Undo taking the lock in the if statement
            return ESP_OK;
        }
        return ESP_ERR_TIMEOUT;
    }

    static esp_err_t lock(std::unique_lock<std::recursive_timed_mutex>& lock)
    {
        lock = std::move(std::unique_lock<std::recursive_timed_mutex>(mutx));
        return ESP_OK;
    }

private:
    static std::recursive_timed_mutex mutx;
};

} // namespace LOGGING

inline LOGGING::Logging LOG;