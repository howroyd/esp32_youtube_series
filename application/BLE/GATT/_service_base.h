#include <array>
#include <cstdint>
#include <cstring>

#pragma once

#include "esp_err.h"

#include "esp_bt_defs.h"
#include "esp_gap_ble_api.h"
#include "esp_gatt_defs.h"
#include "esp_gatts_api.h"

namespace GATT {

static constexpr esp_bt_uuid_t primary_service_uuid = 
    { ESP_UUID_LEN_16, ESP_GATT_UUID_PRI_SERVICE };
static constexpr esp_bt_uuid_t char_declaration_uuid = 
    { ESP_UUID_LEN_16, ESP_GATT_UUID_CHAR_DECLARE };
static constexpr esp_bt_uuid_t char_client_config_uuid = 
    { ESP_UUID_LEN_16, ESP_GATT_UUID_CHAR_CLIENT_CONFIG };

static const esp_gatt_perm_t char_perm_read = ESP_GATT_PERM_READ;

static const esp_gatt_char_prop_t char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;
static const esp_gatt_char_prop_t char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_WRITE | ESP_GATT_CHAR_PROP_BIT_READ;
static const esp_gatt_char_prop_t char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

} // namespace Gatt