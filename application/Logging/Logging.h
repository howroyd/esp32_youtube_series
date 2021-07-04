#pragma once

#include "esp_err.h"
#include "esp_log.h"

#include <chrono>
#include <experimental/source_location>
#include <mutex>
#include <sstream>
#include <string>
#include <utility>

namespace LOGGING
{

/// @brief Multi-core and multi-thread safe logging class
class Logging
{
    using Ms                = std::chrono::milliseconds;                ///< typename alias
    using source_location   = std::experimental::source_location;       ///< typename alias

    constexpr static esp_log_level_t    default_level{ESP_LOG_INFO};    ///< Default logging level if none set
    constexpr static Ms                 defalt_mutex_wait{100};         ///< Default log timeout if none set

public:
    /// @brief Log a message at a given level
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @param[in] level    : level to log this message at
    /// @param[in] msg      : message to log
    /// @param[in] location : (optional) source location where the log originates from
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_INVALID_STATE if timed out waiting to log the message
    static esp_err_t log(const esp_log_level_t level,
                            const std::string_view& msg,
                            const source_location& location = source_location::current())
    {
        if (default_level < level) return ESP_ERR_INVALID_STATE;
        
        if (mutx.try_lock_for(defalt_mutex_wait))
        {
            ESP_LOG_LEVEL(level, location.file_name(), 
                            "[%d:%s]: %.*s", 
                            location.line(), location.function_name(), 
                            msg.length(), msg.data());
            mutx.unlock();
            return ESP_OK;
        }
        return ESP_ERR_TIMEOUT;
    }

    /// @brief Log a message at a given level
    ///
    /// @note Operator overload so we can just call the class name as a function
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @param[in] level    : level to log this message at
    /// @param[in] msg      : message to log
    /// @param[in] location : (optional) source location where the log originates from
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_TIMEOUT if timed out waiting to log the message
    esp_err_t operator() (const esp_log_level_t level,
                            const std::string_view& msg,
                            const source_location& location = source_location::current())
    {
        return log(level, msg, location);
    }

    /// @brief Log an error message (red)
    ///
    /// Times out if the log is busy
    ///
    /// @param[in] msg      : message to log
    /// @param[in] location : (optional) source location where the log originates from
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_TIMEOUT if timed out waiting to log the message
    static esp_err_t error(const std::string_view& msg,
                            const source_location& location = source_location::current())
    {
        return log(ESP_LOG_ERROR, msg, location);
    }

    /// @brief Log a warning message (yellow)
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @param[in] msg      : message to log
    /// @param[in] location : (optional) source location where the log originates from
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_TIMEOUT if timed out waiting to log the message
    static esp_err_t warning(const std::string_view& msg,
                                const source_location& location = source_location::current())
    {
        return log(ESP_LOG_WARN, msg, location);
    }

    /// @brief Log an information message (green)
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @param[in] msg      : message to log
    /// @param[in] location : (optional) source location where the log originates from
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_TIMEOUT if timed out waiting to log the message
    static esp_err_t info(const std::string_view& msg,
                            const source_location& location = source_location::current())
    {
        return log(ESP_LOG_INFO, msg, location);
    }

    /// @brief Log a debug message (white)
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @param[in] msg      : message to log
    /// @param[in] location : (optional) source location where the log originates from
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_TIMEOUT if timed out waiting to log the message
    static esp_err_t debug(const std::string_view& msg,
                            const source_location& location = source_location::current())
    {
        return log(ESP_LOG_DEBUG, msg, location);
    }

    /// @brief Log a verbose message
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @param[in] msg      : message to log
    /// @param[in] location : (optional) source location where the log originates from
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_TIMEOUT if timed out waiting to log the message
    static esp_err_t verbose(const std::string_view& msg,
                                const source_location& location = source_location::current())
    {
        return log(ESP_LOG_VERBOSE, msg, location);
    }

    /// @brief Aquire the logging lock
    ///
    /// Used to block all other threads from logging
    ///
    /// @param[in,out] lock         : std::unique_lock to store the lock if aquired
    /// @param[in] timeout_duration : (optional) time to wait for the lock before failing
    ///
	/// @return 
	/// 	- ESP_OK if lock obtained
    ///     - ESP_ERR_TIMEOUT if timed out waiting for the lock
    template<class Rep, class Period>
    [[nodiscard]] static esp_err_t lock(std::unique_lock<std::recursive_timed_mutex>& lock, 
                                        const std::chrono::duration<Rep, Period>& timeout_duration = Ms(100))
    {
        if (mutx.try_lock_for(timeout_duration))
        {
            lock = std::move(std::unique_lock<std::recursive_timed_mutex>(mutx));
            mutx.unlock(); // Undo taking the lock in the if statement
            return ESP_OK;
        }
        return ESP_ERR_TIMEOUT;
    }

    /// @brief Aquire the logging lock
    ///
    /// @attention Will block until the lock is obtained
    ///
    /// Used to block all other threads from logging
    ///
    /// @param[in,out] lock : std::unique_lock to store the lock if aquired
    ///
	/// @return ESP_OK
    static esp_err_t lock(std::unique_lock<std::recursive_timed_mutex>& lock)
    {
        lock = std::move(std::unique_lock<std::recursive_timed_mutex>(mutx));
        return ESP_OK;
    }

private:
    esp_log_level_t instance_level{default_level};          ///< Minimum log level for this instance
    Ms              instance_mutex_wait{defalt_mutex_wait}; ///< Maximum log lock wait time for this instance

public:
    Logging(void) noexcept = default;                       ///< Trivially constructable

    /// @brief Construct a logging instance
    ///
    /// @param[in] level            : minimum log level for this instance
    /// @param[in] mutex_wait_ms    : maximum log lock wait time for this instance
    constexpr Logging(const esp_log_level_t level, const Ms mutex_wait_ms) noexcept :
        instance_level{level},
        instance_mutex_wait{mutex_wait_ms}
    {}

    /// @brief Construct a logging instance
    ///
    /// @param[in] level            : minimum log level for this instance
    constexpr Logging(const esp_log_level_t level) noexcept :
        Logging{level, defalt_mutex_wait}
    {}

    /// @brief Construct a logging instance
    ///
    /// @param[in] mutex_wait_ms    : maximum log lock wait time for this instance
    constexpr Logging(const Ms mutex_wait_ms) noexcept :
        Logging{default_level, mutex_wait_ms}
    {}

private:
    static std::recursive_timed_mutex mutx; ///< API access mutex

    struct FormatWithLocation 
    {
        std::string_view    format;
        source_location     location;

        FormatWithLocation(const std::string_view&& format,
                            const source_location& location = source_location::current())
            : format{format}, location{location}
        {}
    }; ///< Container to hold a string view and an optional source location

    template <typename T>
    static void args_to_stream(std::ostream& stream, const T arg)
    {
        constexpr static const char* delimiter{" "};
        stream << arg << delimiter;
    }

    template<typename T, typename... Args>
    static void args_to_stream(std::ostream& stream, const T arg, const Args... args)
    {
        args_to_stream(stream, arg);
        args_to_stream(stream, args...);
    }

public:
    template <typename... Args>
    static esp_err_t vlog(const esp_log_level_t level, 
                            const FormatWithLocation&& format, 
                            const Args... args)
    {
        std::ostringstream stream;
        args_to_stream(stream, args...);
        return log(level, stream.str(), format.location);
    }
};

inline std::recursive_timed_mutex Logging::mutx{}; ///< API access mutex

} // namespace LOGGING

inline LOGGING::Logging LOG; ///< Must be inline to ensure it is only constructed once