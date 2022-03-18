#pragma once

#include <array>
#include <functional>
#include <memory>
#include <type_traits>

#include "esp_gatt_defs.h"

using TypeId = void*;

template<class T>
TypeId TypeIdNoRTTI(void)
{
    static T* TypeUniqueMarker = nullptr;
    return &TypeUniqueMarker;
}

namespace GATT {

#define MAX_LEN 20 // bytes

struct attr_desc_wrapper_t
{
    esp_bt_uuid_t               uuid;
    esp_gatt_perm_t             perm;
    uint16_t                    max_length;
    uint16_t                    length;
    std::unique_ptr<uint8_t[]>  value;
    TypeId                      typeID = TypeIdNoRTTI<void>();

    esp_attr_desc_t get(void) const noexcept
    {
        return {
            uuid.len,
            const_cast<uint8_t*>(uuid.uuid.uuid128),
            perm,
            max_length,
            length,
            value.get()
        };
    }
};

template <size_t n_elements>
using gatt_table_t = std::array<attr_desc_wrapper_t, n_elements>;

template <size_t n_elements, class... Attrs>
gatt_table_t<n_elements> make_table(Attrs&&... attrs)
{
    return {(std::move(attrs), ...)};
}

template <class T>
using strip_ptr_type = std::remove_pointer_t<std::remove_reference_t<T>>;

template <class T>
using strip_arr_type = std::remove_pointer_t<std::remove_extent_t<std::remove_reference_t<T>>>;

template <class T, size_t n_elements = 1>
struct attr_value_t
{
    static constexpr size_t len         = n_elements;
    static constexpr size_t len_type    = sizeof(strip_ptr_type<T>);
    static constexpr size_t len_bytes   = len * len_type;
    static_assert(len_bytes);

    std::unique_ptr<uint8_t[]> val;
    TypeId typeID = TypeIdNoRTTI<strip_ptr_type<T>>();

    attr_value_t(const T value) :
        val{new uint8_t[len_bytes]}
    {
        const uint8_t* p = nullptr;

        if constexpr (std::is_pointer_v<T> || std::is_array_v<T>)
            p = reinterpret_cast<const uint8_t*>(value);
        else
            p = reinterpret_cast<const uint8_t*>(&value);

        std::array<uint8_t, len_bytes> arr;
        for (auto& elem : arr) elem = *(p++);

        std::copy(arr.begin(), arr.end(), val.get());
    }
};

template <>
struct attr_value_t<void>
{
    static constexpr size_t len         = 0;
    static constexpr size_t len_type    = 0;
    static constexpr size_t len_bytes   = 0;

    std::unique_ptr<uint8_t[]> val;
    TypeId typeID = TypeIdNoRTTI<void>();
};

template <class T, size_t n_elements = 1>
struct table_row_t
{
    using UnderlyingType = std::remove_reference_t<T>;

    esp_bt_uuid_t                   uuid;
    esp_gatt_perm_t                 permission;
    attr_value_t<UnderlyingType>    value;
    uint16_t                        max_len = MAX_LEN;
};

template <typename... Ts>
gatt_table_t<sizeof...(Ts)> make_table_from_rows(Ts&&... values)
// requires Ts to be table_row_t<...>
{
    gatt_table_t<sizeof...(Ts)> ret;

    size_t idx = 0;
    auto fill_arr_one = [&ret, &idx](auto& arg)
    {
        ret[idx++] = {  arg.uuid,
                        arg.permission,
                        arg.max_len,
                        arg.value.len_bytes,
                        std::move(arg.value.val),
                        arg.value.typeID
        };
    };

    (fill_arr_one(values), ...);

    return ret;
}

} // namespace GATT