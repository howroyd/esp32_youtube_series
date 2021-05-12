#pragma once

#include <cstring>

#include "esp_err.h"
#include "nvs_flash.h"
#include "nvs.h"
	/// @brief Create a GATT service in the API
	///
	/// @param[in] gatt_if : The GATTS Interface ID that this service will reside in
	/// @param[in] primary_uuid : UUID of the primary service of this table
	/// @param[in] attributes... : Attributes created with attr_characteristic
	/// @return 
	/// 	- ESP_OK if service table created in API
	/// 	- ESP_ERR_INVALID_STATE if service aleady created
	/// 	- ESP_ERR_NO_MEM if heap allocations failed
	/// 	- other error codes from underlying GATTS API
namespace NVS
{

class Nvs
{
    const char* const   _log_tag{nullptr};
    nvs_handle_t        handle{};
    const char* const   partition_name{nullptr};

public:
	/// @brief Construct a non-volatile storage interface
	///
	/// @param[in] partition_name : cstring of partition name as defined in the partition table
    constexpr Nvs(const char* const partition_name = "nvs") :
        _log_tag{partition_name},
        partition_name{partition_name} {}

	/// @brief Open the partition
	///
	/// @return 
	/// 	- ESP_OK if partition opened
	/// 	- other error codes from underlying NVS API
    [[nodiscard]] esp_err_t init(void)
        { return _open(partition_name, handle); }

	/// @brief Get an item from the NVS
	///
	/// @param[in] key : cstring key referring to an item in NVS
	/// @param[out] output : variable to write the item to
	/// @return 
	/// 	- ESP_OK if the item was read from NVS
	/// 	- other error codes from underlying NVS API
    template <typename T>
    [[nodiscard]] esp_err_t get(const char* const key, T& output)
        { return _get_buf(handle, key, output, 1); }

	/// @brief Set an item in the NVS
	///
	/// @param[in] key : cstring key of new or existing item in NVS
	/// @param[in] input : variable to write
	/// @return 
	/// 	- ESP_OK if the item was written and verified
	/// 	- other error codes from underlying NVS API
    template <typename T>
    [[nodiscard]] esp_err_t set(const char* const key, const T input)
        { return _set_buf(handle, key, input, 1); }

    template <typename T>
    [[nodiscard]] esp_err_t verify(const char* const key, const T input)
        { return _verify_buf(handle, key, input, 1); }


    // With respect to a buffer
    template <typename T>
    [[nodiscard]] esp_err_t get_buffer(const char* const key, T* output, size_t& len)
        { return _get_buf(handle, key, output, len); }

    template <typename T>
    [[nodiscard]] esp_err_t set_buffer(const char* const key, const T* input, const size_t len)
        { return _set_buf(handle, key, input, len); }

    template <typename T>
    [[nodiscard]] esp_err_t verify_buffer(const char* const key, const T* input, const size_t len)
        { return _verify_buf(handle, key, input, len); }

private:
    [[nodiscard]] static esp_err_t _open(const char* const partition_name, nvs_handle_t& handle)
        { return nvs_open(partition_name, NVS_READWRITE, &handle); }

    template <typename T>
    [[nodiscard]] static esp_err_t _get(nvs_handle_t handle, const char* const key, T& output)
    {
        size_t n_bytes{sizeof(T)};

        if (nullptr == key || 0 == strlen(key))
            return ESP_ERR_INVALID_ARG;
        else
            return _get_buffer(handle, key, &output, n_bytes);
    }

    template <typename T>
    [[nodiscard]] static esp_err_t _set(nvs_handle_t handle, const char* const key, T& input)
    {
        size_t n_bytes{sizeof(T)};

        if (nullptr == key || 0 == strlen(key))
            return ESP_ERR_INVALID_ARG;
        else
        {
            esp_err_t status{_set_buffer(handle, key, &input, n_bytes)};

            if (ESP_OK == status) status |= nvs_commit(handle);

            if (ESP_OK == status) status |= _verify(handle, key, &input, n_bytes);

            return status;
        }
    }

    template <typename T>
    [[nodiscard]] static esp_err_t _verify(nvs_handle_t handle, const char* const key, T& input)
    {
        T val_in_nvs{};
        esp_err_t status = _get(handle, key, val_in_nvs);

        if (ESP_OK == status)
            if (input == val_in_nvs)
                return ESP_OK;
            else
                return ESP_FAIL;
        else
            return status;
    }

    // Note, length of num items of type T (not necessarily num bytes)
    template <typename T>
    [[nodiscard]] static esp_err_t _get_buf(nvs_handle_t handle, const char* const key, 
                                            T* output, size_t& len)
    {
        size_t n_bytes{sizeof(T) * len};

        if (nullptr == key || 0 == strlen(key) || nullptr == output || 0 == len)
            return ESP_ERR_INVALID_ARG;
        else
        {
            const esp_err_t status = nvs_get_blob(handle, key, output, &n_bytes);

            len = n_bytes / sizeof(T);

            return status;
        }
    }

    template <typename T>
    [[nodiscard]] static esp_err_t _set_buf(nvs_handle_t handle, const char* const key, 
                                            const T* input, const size_t len)
    {
        size_t n_bytes{sizeof(T) * len};
        esp_err_t status{ESP_OK};

        if (nullptr == key || 0 == strlen(key) || nullptr == input || 0 == len)
            status = ESP_ERR_INVALID_ARG;
        else
        {
            status = nvs_set_blob(handle, key, input, &n_bytes);

            if (ESP_OK == status)
                status = nvs_commit(handle);

            if (ESP_OK == status)
                status = _verify_buf(handle, key, input, len);
        }

        return status;
    }

    template <typename T>
    [[nodiscard]] static esp_err_t _verify_buf(nvs_handle_t handle, const char* const key, 
                                                const T* input, const size_t len)
    {
        esp_err_t status{ESP_OK};

        T* buf_in_nvs = new T[len]{};
        size_t n_items_in_nvs = len;

        if (buf_in_nvs)
        {
            status = _get_buf(handle, key, buf_in_nvs, n_items_in_nvs);

            if (ESP_OK == status)
            {
                if (len == n_items_in_nvs)
                {
                    if (0 != memcmp(input, buf_in_nvs, len * sizeof(T)))
                        status = ESP_FAIL;
                }
                else
                    status = ESP_ERR_NVS_INVALID_LENGTH;
            }

            delete[] buf_in_nvs;
        }
        else
        {
            status = ESP_ERR_NO_MEM;
        }

        return status;
    }

};

} // namespace NVS