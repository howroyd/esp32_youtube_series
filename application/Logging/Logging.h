#pragma once

#include "esp_err.h"
#include "esp_log.h"

#include <cstring>
#include <chrono>
#include <experimental/source_location>
#include <mutex>
#include <sstream>
#include <string>
#include <type_traits>
#include <utility>

namespace LOGGING
{

/// @brief Multi-core and multi-thread safe logging class
class Logging
{
public:
    using
    Ms              = std::chrono::milliseconds;            ///< typename alias
    
    using
    source_location = std::experimental::source_location;   ///< typename alias

private:
    using
    mutx_t          = std::recursive_timed_mutex;           ///< typename alias for the mutex used

    constexpr static
    esp_log_level_t default_level{ESP_LOG_INFO};            ///< Default logging level if none set

    constexpr static
    Ms              defalt_mutex_wait{100};                 ///< Default log timeout if none set

    esp_log_level_t instance_level{default_level};          ///< Minimum log level for this instance
    Ms              instance_mutex_wait{defalt_mutex_wait}; ///< Maximum log lock wait time for this instance

    static
    mutx_t          mutx;                                   ///< API access mutex

    constexpr static 
    size_t          logf_buf_len{1024};                     ///< snprintf logbuf length

    template<typename... Args>
    class           GetLast;                                ///< Forward declaration of helper class

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

        const char* file_name_trimmed = strrchr(location.file_name(), '/') + 1;
        const char* func_name_trimmed = strchr(location.function_name(), '<');
        
        if (mutx.try_lock_for(defalt_mutex_wait))
        {
            ESP_LOG_LEVEL(level, file_name_trimmed ? file_name_trimmed : location.file_name(), 
                            "[%d:%.*s]: %.*s", 
                            location.line(), 
                            func_name_trimmed ? (func_name_trimmed - location.function_name()) :
                                                 strlen(location.function_name()),
                            location.function_name(), 
                            msg.length(), msg.data());
            mutx.unlock();
            return ESP_OK;
        }
        return ESP_ERR_TIMEOUT;
    }

    /// @brief Log a message at a given level
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @param[in] level : level to log this message at
    /// @param[in] args  : (variadic) args to be passed to the std::stringstream
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_INVALID_STATE if timed out waiting to log the message
    template <typename... Args>
    static esp_err_t logv(const esp_log_level_t level, 
                            const Args&... args)
    {
        std::ostringstream stream;
        args_to_stream(stream, args...);
        return log(level, stream.str());
    }

    /// @brief Log a printf style message at a given level
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @note This is the version when source_location is the last arg
    ///
    /// @param[in] level  : level to log this message at
    /// @param[in] format : printf format cstring
    /// @param[in] args   : (variadic) args to be passed to the std::stringstream.  Last arg can be a source location
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_INVALID_STATE if timed out waiting to log the message
    template <typename... Args,
                std::enable_if_t<
                    std::is_same_v<
                        typename GetLast<Args...>::type, 
                        source_location>,
                    bool> = true
            >
    static esp_err_t logf(const esp_log_level_t level, const char* const format, const Args&... args)
    {
        char buf[logf_buf_len]{};
        snprintf(buf, sizeof(buf), format, args...); // FIXME this forwards the sourcelocation arg which will fail
        return log(level, buf, GetLast(args...).value);
    }

    /// @brief Log a printf style message at a given level
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @note This is the version when source_location is not included as the last arg
    ///
    /// @param[in] level  : level to log this message at
    /// @param[in] format : printf format cstring
    /// @param[in] args   : (variadic) args to be passed to the std::stringstream.  Last arg can be a source location
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_INVALID_STATE if timed out waiting to log the message
    template <typename... Args,
                std::enable_if_t<
                    !std::is_same_v<
                        typename GetLast<Args...>::type, 
                        source_location>,
                    bool> = true
            >
    static esp_err_t logf(const esp_log_level_t level, const char* const format, const Args&... args)
    {
        char buf[logf_buf_len]{};
        snprintf(buf, sizeof(buf), format, args...);
        return log(level, buf);
    }

    /// @brief Log a buffer of hex bytes at specified level, separated into 16 bytes each line.
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @param[in] level    : level to log this message at
    /// @param[in] buf      : buffer to log
    /// @param[in] buf_len  : buffer length in bytes
    /// @param[in] location : (optional) source location where the log originates from
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    /// 	- ESP_ERR_INVALID_ARG if buffer is nullptr or length is zero
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_INVALID_STATE if timed out waiting to log the message
    static esp_err_t hex(const esp_log_level_t level,
                            const void* buf, const size_t buf_len,
                            const source_location& location = source_location::current())
    {
        if (default_level < level) return ESP_ERR_INVALID_STATE;
        
        if (buf && 0 < buf_len)
        {
            if (mutx.try_lock_for(defalt_mutex_wait))
            {
                ESP_LOG_BUFFER_HEX_LEVEL(location.file_name(), 
                                            buf, buf_len, level);
                mutx.unlock();
                return ESP_OK;
            }
            return ESP_ERR_TIMEOUT;
        }
        return ESP_ERR_INVALID_ARG;
    }

    /// @brief Dump a buffer to the log at specified level
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @param[in] level    : level to log this message at
    /// @param[in] buf      : buffer to log
    /// @param[in] buf_len  : buffer length in bytes
    /// @param[in] location : (optional) source location where the log originates from
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    /// 	- ESP_ERR_INVALID_ARG if buffer is nullptr or length is zero
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_INVALID_STATE if timed out waiting to log the message
    static esp_err_t hexdump(const esp_log_level_t level,
                                const void* buf, const size_t buf_len,
                                const source_location& location = source_location::current())
    {
        if (default_level < level) return ESP_ERR_INVALID_STATE;
        
        if (buf && 0 < buf_len)
        {
            if (mutx.try_lock_for(defalt_mutex_wait))
            {
                ESP_LOG_BUFFER_HEXDUMP(location.file_name(), 
                                            buf, buf_len, level);
                mutx.unlock();
                return ESP_OK;
            }
            return ESP_ERR_TIMEOUT;
        }
        return ESP_ERR_INVALID_ARG;
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

    /// @brief Log a message at a given level
    ///
    /// @note Operator overload so we can just call the class name as a function
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @param[in] level : level to log this message at
    /// @param[in] args  : (variadic) args to be passed to the std::stringstream.  Last arg can be a source location
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_TIMEOUT if timed out waiting to log the message
    template <typename... Args>
    esp_err_t operator() (const esp_log_level_t level, 
                            const Args&... args)
    {
        return logv(level, args...);
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

    /// @brief Log an error message (red)
    ///
    /// Times out if the log is busy
    ///
    /// @param[in] args  : (variadic) args to be passed to the std::stringstream.  Last arg can be a source location
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_TIMEOUT if timed out waiting to log the message
    template <typename... Args>
    static esp_err_t errorv(const Args&... args)
    {
        return logv(ESP_LOG_ERROR, args...);
    }

    /// @brief Log a printf style error message (red)
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @note This is the version when source_location is not included as the last arg
    ///
    /// @param[in] format : printf format cstring
    /// @param[in] args   : (variadic) args to be passed to the std::stringstream.  Last arg can be a source location
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_INVALID_STATE if timed out waiting to log the message
    template <typename... Args>
    static esp_err_t errorf(const char* const format, const Args&... args)
    {
        return logf(ESP_LOG_ERROR, format, args...);
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

    /// @brief Log a warning message (yellow)
    ///
    /// Times out if the log is busy
    ///
    /// @param[in] args  : (variadic) args to be passed to the std::stringstream.  Last arg can be a source location
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_TIMEOUT if timed out waiting to log the message
    template <typename... Args>
    static esp_err_t warningv(const Args&... args)
    {
        return logv(ESP_LOG_WARN, args...);
    }

    /// @brief Log a printf style warning message (yellow)
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @note This is the version when source_location is not included as the last arg
    ///
    /// @param[in] format : printf format cstring
    /// @param[in] args   : (variadic) args to be passed to the std::stringstream.  Last arg can be a source location
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_INVALID_STATE if timed out waiting to log the message
    template <typename... Args>
    static esp_err_t warningf(const char* const format, const Args&... args)
    {
        return logf(ESP_LOG_WARN, format, args...);
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

    /// @brief Log an information message (green)
    ///
    /// Times out if the log is busy
    ///
    /// @param[in] args  : (variadic) args to be passed to the std::stringstream.  Last arg can be a source location
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_TIMEOUT if timed out waiting to log the message
    template <typename... Args>
    static esp_err_t infov(const Args&... args)
    {
        return logv(ESP_LOG_INFO, args...);
    }

    /// @brief Log a printf style information message (green)
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @note This is the version when source_location is not included as the last arg
    ///
    /// @param[in] format : printf format cstring
    /// @param[in] args   : (variadic) args to be passed to the std::stringstream.  Last arg can be a source location
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_INVALID_STATE if timed out waiting to log the message
    template <typename... Args>
    static esp_err_t infof(const char* const format, const Args&... args)
    {
        return logf(ESP_LOG_INFO, format, args...);
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

    /// @brief Log a debug message (white)
    ///
    /// Times out if the log is busy
    ///
    /// @param[in] args  : (variadic) args to be passed to the std::stringstream.  Last arg can be a source location
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_TIMEOUT if timed out waiting to log the message
    template <typename... Args>
    static esp_err_t debugv(const Args&... args)
    {
        return logv(ESP_LOG_DEBUG, args...);
    }

    /// @brief Log a printf style debug message (white)
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @note This is the version when source_location is not included as the last arg
    ///
    /// @param[in] format : printf format cstring
    /// @param[in] args   : (variadic) args to be passed to the std::stringstream.  Last arg can be a source location
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_INVALID_STATE if timed out waiting to log the message
    template <typename... Args>
    static esp_err_t debugf(const char* const format, const Args&... args)
    {
        return logf(ESP_LOG_DEBUG, format, args...);
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

    /// @brief Log a verbose message
    ///
    /// Times out if the log is busy
    ///
    /// @param[in] args  : (variadic) args to be passed to the std::stringstream.  Last arg can be a source location
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_TIMEOUT if timed out waiting to log the message
    template <typename... Args>
    static esp_err_t verbosev(const Args&... args)
    {
        return logv(ESP_LOG_VERBOSE, args...);
    }

    /// @brief Log a printf style verbose message
    ///
    /// Will only log if above the default logging level
    /// Times out if the log is busy
    ///
    /// @note This is the version when source_location is not included as the last arg
    ///
    /// @param[in] format : printf format cstring
    /// @param[in] args   : (variadic) args to be passed to the std::stringstream.  Last arg can be a source location
    ///
	/// @return 
	/// 	- ESP_OK if message logged
    ///     - ESP_ERR_INVALID_STATE if requested level is below our default minimum
    ///     - ESP_ERR_INVALID_STATE if timed out waiting to log the message
    template <typename... Args>
    static esp_err_t verbosef(const char* format, const Args&... args)
    {
        return logf(ESP_LOG_VERBOSE, format, args...);
    }

public:
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
    [[nodiscard]] static esp_err_t lock(std::unique_lock<mutx_t>& lock, 
                                        const std::chrono::duration<Rep, Period>& timeout_duration = Ms(100))
    {
        if (mutx.try_lock_for(timeout_duration))
        {
            lock = std::move(std::unique_lock<mutx_t>(mutx));
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
    static esp_err_t lock(std::unique_lock<mutx_t>& lock)
    {
        lock = std::move(std::unique_lock<mutx_t>(mutx));
        return ESP_OK;
    }

private:
    /// @brief Append an argument to a output stream
    ///
    /// @param[in,out] stream : std::ostream to append the argument to
    /// @param[in]     arg    : argument to append to the ostream
    ///
	/// @return stream
    template <typename T>
    static std::ostream& args_to_stream(std::ostream& stream, const T& arg)
    {
        constexpr static const char* delimiter{" "};
        return stream << arg << delimiter;
    }

    /// @brief Append multiple arguments to a output stream
    ///
    /// @param[in,out] stream : std::ostream to append the argument to
    /// @param[in]     arg    : (variadic) arguments to append to the ostream
    ///
	/// @return stream
    template<typename T, typename... Args>
    static std::ostream& args_to_stream(std::ostream& stream, const T& arg, const Args&... args)
    {
        return args_to_stream(args_to_stream(stream, arg), args...);
    }

    /// @brief Helper class for compile time argument parsing 
    template<typename... Args>
    class GetLast
    {
        template<typename T>
        struct tag
        { using type = T; }; ///< Extract the type of an argument as a thing with a name

        /// @brief Return an argument
        /// @note Required for the variadic template below to work
        template <typename T>
        constexpr T last_value(const T&& val) 
        { return val; }

        /// @brief Return the last argument of a pack
        template <typename T, typename... Ts>
        constexpr auto last_value(const T&& val, const Ts&&... vals)
        { return last_value(vals...); }

    public:
        // Use a fold-expression to fold the comma operator over the parameter pack.
        using type = typename decltype((tag<Args>{}, ...))::type;
        
        const type value; ///< The value of the last argument in the pack

        /// @brief Constructor
        ///
        /// @param[in] args : (variadic) arguement pack
        constexpr GetLast(const Args&&... args) : 
            value{last_value(args...)}
        {}
    };

};

inline Logging::mutx_t Logging::mutx{}; ///< API access mutex

} // namespace LOGGING

inline LOGGING::Logging LOG; ///< Must be inline to ensure it is only constructed once