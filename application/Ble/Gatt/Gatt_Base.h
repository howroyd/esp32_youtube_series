#include <cstdint>
#include <cstring>

#pragma once

#include "esp_err.h"

#include "esp_gap_ble_api.h"
#include "esp_gatts_api.h"
#include "esp_gatt_defs.h"
#include "esp_bt_defs.h"

#define DATA_MAX_LEN 20

namespace Gatt
{

// --------------------------------------------------------------------------------------------------------------------
// GATT Base Class
// --------------------------------------------------------------------------------------------------------------------
// Characteristics require two table entries, notifications require three
//   This is a template to ensure stack allocation, rather than having to use
//   the "new" operator and construct on the heap.
template <uint16_t n_characteristics, uint16_t n_notifies = 0>
class Gatt_table_base
{
public:
	// API call to copy our table onto the BLE stack
	esp_err_t create_table(esp_gatt_if_t gatts_if, const bool override = false)
	{
		esp_err_t status = ESP_OK;

		if (override || !created)
		{
			status |= esp_ble_gatts_create_attr_tab(table, gatts_if, n_entries, id);
			created = true;
		}

		return status;
	}

	// API call to start the service and aquire the service handles
	esp_err_t start_service(const uint16_t* handle_table, const bool override = false)
	{
		esp_err_t status = ESP_OK;

		if (override || !started)
		{
			memcpy(this->handle_table, handle_table, sizeof(this->handle_table));

			status |= esp_ble_gatts_start_service(handle_table[0]);
			started = true;
		}

		return status;
	}

	// API call to update a characteristic value buffer on the BLE stack
	esp_err_t update_value(const uint16_t id, const uint8_t* buf, const uint16_t buf_len)
	{
		return esp_ble_gatts_set_attr_value(handle_table[id], buf_len, buf);
	}

	// Service primary UUID
	const esp_bt_uuid_t uuid_primary;
	
	// Total number of entries in this service table
	const uint16_t n_entries = 1 + (2 * n_characteristics) + (3 * n_notifies);

	// This service table unique ID
	const uint16_t id;

	// Operator overload to return a handle
	uint16_t operator[] (uint16_t handle_id) const
	{
		return handle_table[handle_id];
	}

	// Method to return true if table has been copied to the BLE stack
	bool table_created(void) const
	{
		return created;
	}

	// Method to return true if service has been started and is running
	bool service_started(void) const
	{
		return started;
	}

protected:
	// Main GATT service table.  Variable length based upon template params,
	//   number of characteristics (2 entries per) and number of
	//   notifiy characteristics (3 entries per) plus the primary service.
	esp_gatts_attr_db_t table[1 + (2 * n_characteristics) + (3 * n_notifies)];

	// Handles to the BLE stack
	uint16_t handle_table[1 + (2 * n_characteristics) + (3 * n_notifies)];

	bool created = false;
	bool started = false;

	Gatt_table_base(const esp_bt_uuid_t uuid, const uint16_t table_id) :
		uuid_primary(uuid),
		id(table_id)
	{
		;
	}

	// BLE service UUID definitions from the BLE standard
	static const esp_bt_uuid_t pri_serv_uuid;// = {ESP_UUID_LEN_16, ESP_GATT_UUID_PRI_SERVICE};
	static const esp_bt_uuid_t char_dec_uuid;// = {ESP_UUID_LEN_16, ESP_GATT_UUID_CHAR_DECLARE};
	static const esp_bt_uuid_t char_client_conf_uuid;// = {ESP_UUID_LEN_16, ESP_GATT_UUID_CHAR_CLIENT_CONFIG};

	// BLE characteristic properties
	static const esp_gatt_perm_t char_prop_read;// = ESP_GATT_CHAR_PROP_BIT_READ;
	static const esp_gatt_perm_t char_prop_read_write;// = ESP_GATT_CHAR_PROP_BIT_WRITE_NR | ESP_GATT_CHAR_PROP_BIT_READ;
	static const esp_gatt_perm_t char_prop_read_notify;// = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

	// BLE characteristic definitions from the BLE standard without our own defined permissions
	// Read only characteristic
	static const esp_gatts_attr_db_t attr_entry_char_dec_read;
	// Writable (by connected device) characteristic
	static const esp_gatts_attr_db_t attr_entry_char_dec_read_write;
	// Notification characteristic
	static const esp_gatts_attr_db_t attr_entry_char_dec_read_notify;
};
// --------------------------------------------------------------------------------------------------------------------

// BLE service UUID definitions from the BLE standard
template <uint16_t n_characteristics, uint16_t n_notifies>
const esp_bt_uuid_t Gatt_table_base<n_characteristics, n_notifies>::pri_serv_uuid = {ESP_UUID_LEN_16, ESP_GATT_UUID_PRI_SERVICE};

template <uint16_t n_characteristics, uint16_t n_notifies>
const esp_bt_uuid_t Gatt_table_base<n_characteristics, n_notifies>::char_dec_uuid = {ESP_UUID_LEN_16, ESP_GATT_UUID_CHAR_DECLARE};

template <uint16_t n_characteristics, uint16_t n_notifies>
const esp_bt_uuid_t Gatt_table_base<n_characteristics, n_notifies>::char_client_conf_uuid = {ESP_UUID_LEN_16, ESP_GATT_UUID_CHAR_CLIENT_CONFIG};

// BLE characteristic properties
template <uint16_t n_characteristics, uint16_t n_notifies>
const esp_gatt_perm_t Gatt_table_base<n_characteristics, n_notifies>::char_prop_read = ESP_GATT_CHAR_PROP_BIT_READ;

template <uint16_t n_characteristics, uint16_t n_notifies>
const esp_gatt_perm_t Gatt_table_base<n_characteristics, n_notifies>::char_prop_read_write = ESP_GATT_CHAR_PROP_BIT_WRITE_NR | ESP_GATT_CHAR_PROP_BIT_READ;

template <uint16_t n_characteristics, uint16_t n_notifies>
const esp_gatt_perm_t Gatt_table_base<n_characteristics, n_notifies>::char_prop_read_notify = ESP_GATT_CHAR_PROP_BIT_READ | ESP_GATT_CHAR_PROP_BIT_NOTIFY;

// BLE characteristic definitions from the BLE standard without our own defined permissions
// Read only characteristic
template <uint16_t n_characteristics, uint16_t n_notifies>
const esp_gatts_attr_db_t Gatt_table_base<n_characteristics, n_notifies>::attr_entry_char_dec_read =
{
	{ESP_GATT_AUTO_RSP},
	{char_dec_uuid.len, (uint8_t*)&char_dec_uuid.uuid,
		ESP_GATT_PERM_READ,
		sizeof(uint8_t), sizeof(uint8_t), (uint8_t*)&char_prop_read},
};

// Writable (by connected device) characteristic
template <uint16_t n_characteristics, uint16_t n_notifies>
const esp_gatts_attr_db_t Gatt_table_base<n_characteristics, n_notifies>::attr_entry_char_dec_read_write =
{
	{ESP_GATT_AUTO_RSP},
	{char_dec_uuid.len, (uint8_t*)&char_dec_uuid.uuid,
		ESP_GATT_PERM_READ,
		sizeof(uint8_t), sizeof(uint8_t), (uint8_t*)&char_prop_read_write},
};
// Notification characteristic
template <uint16_t n_characteristics, uint16_t n_notifies>
const esp_gatts_attr_db_t Gatt_table_base<n_characteristics, n_notifies>::attr_entry_char_dec_read_notify =
{
	{ESP_GATT_AUTO_RSP},
	{char_dec_uuid.len, (uint8_t*)&char_dec_uuid.uuid,
		ESP_GATT_PERM_READ,
		sizeof(uint8_t), sizeof(uint8_t), (uint8_t*)&char_prop_read_notify},
};

} // namespace Gatt
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
