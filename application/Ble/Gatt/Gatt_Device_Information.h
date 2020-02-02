#pragma once

#include "Gatt_Base.h"

namespace Gatt
{

// --------------------------------------------------------------------------------------------------------------------
// Device Information Service
// --------------------------------------------------------------------------------------------------------------------
#define GATT_DEV_INFO_N_ENTRIES 5
class Gatt_dev_info_svc final : public Gatt_table_base<GATT_DEV_INFO_N_ENTRIES>
{
	const char* const manuf;
	const char* const model;
	const char* const serial;
	const char* const hw;
	const char* const fw;

public:
	Gatt_dev_info_svc(const uint16_t table_id,
						const char* const manufacturer,
						const char* const model,
						const char* const serial_number,
						const char* const hardware_ver,
						const char* const firmware_ver) :
			Gatt_table_base(
				esp_bt_uuid_t{
					.len = ESP_UUID_LEN_16,
					.uuid = {.uuid16 = ESP_GATT_UUID_DEVICE_INFO_SVC}
				}, table_id),
			manuf(manufacturer),
			model(model),
			serial(serial_number),
			hw(hardware_ver),
			fw(firmware_ver),
			max_length{static_cast<uint16_t>(strlen(manuf)),
						static_cast<uint16_t>(strlen(model)),
						static_cast<uint16_t>(strlen(serial)),
						static_cast<uint16_t>(strlen(hw)),
						static_cast<uint16_t>(strlen(fw))},
			length{static_cast<uint16_t>(strlen(manuf)),
					static_cast<uint16_t>(strlen(model)),
					static_cast<uint16_t>(strlen(serial)),
					static_cast<uint16_t>(strlen(hw)),
					static_cast<uint16_t>(strlen(fw))},
			value{const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(manuf)),
					const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(model)),
					const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(serial)),
					const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(hw)),
					const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(fw))}
	{
		table[0] = 
		{ 
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {pri_serv_uuid.len, (uint8_t*)&pri_serv_uuid.uuid, ESP_GATT_PERM_READ, uuid_primary.len, uuid_primary.len, (uint8_t*)&uuid_primary.uuid}
		};
		table[1] = attr_entry_char_dec_read;
		table[IDX_MAUF_VAL] = 
		{
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {uuid_list[0].len, (uint8_t*)&uuid_list[0].uuid, permissions[0], max_length[0], length[0], (uint8_t*)value[0]}
		};
		table[3] = attr_entry_char_dec_read;
		table[IDX_MODEL_VAL] = 
		{
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {uuid_list[1].len, (uint8_t*)&uuid_list[1].uuid, permissions[1], max_length[1], length[1], (uint8_t*)value[1]}
		};
		table[5] = attr_entry_char_dec_read;
		table[IDX_SERIAL_VAL] = 
		{
			.attr_control = {ESP_GATT_AUTO_RSP},
			.att_desc = {uuid_list[2].len, (uint8_t*)&uuid_list[2].uuid, permissions[2], max_length[2], length[2], (uint8_t*)value[2]}
		};
		table[7] = attr_entry_char_dec_read;
		table[IDX_HW_VAL] = 
		{
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {uuid_list[3].len, (uint8_t *)&uuid_list[3].uuid, permissions[3], max_length[3], length[3], (uint8_t*)value[3]}
		};
		table[9] = attr_entry_char_dec_read;
		table[IDX_FW_VAL] = 
		{
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {uuid_list[4].len, (uint8_t *)&uuid_list[4].uuid, permissions[4], max_length[4], length[4], (uint8_t*)value[4]}
		};
	}

	enum
	{
		IDX_MAUF_VAL = 2,
		IDX_MODEL_VAL = 4,
		IDX_SERIAL_VAL = 6,
		IDX_HW_VAL = 8,
		IDX_FW_VAL = 10
	};

	// Method to change the Hub serial number entry on the BLE stack
	bool change_serial(const uint32_t serial_24bit)
	{
		char serial[7];
		snprintf(serial, sizeof(serial), "%06X", serial_24bit);

		return ESP_OK == update_value(IDX_SERIAL_VAL, (uint8_t*)serial, sizeof(serial) - 1);
	}
	bool change_serial(const uint8_t serial_24bit[6])
	{
		char serial[7];
		snprintf(serial, sizeof(serial), "%02X%02X%02X", serial_24bit[3], serial_24bit[4], serial_24bit[5]);

		return ESP_OK == update_value(IDX_SERIAL_VAL, (uint8_t*)serial, sizeof(serial) - 1);
	}

private:
	// UUIDs of characteristics within the device information service from the BLE standard
	static const esp_bt_uuid_t uuid_list[GATT_DEV_INFO_N_ENTRIES];
	static const esp_gatt_perm_t permissions[GATT_DEV_INFO_N_ENTRIES];

	const uint16_t max_length[GATT_DEV_INFO_N_ENTRIES];
	const uint16_t length[GATT_DEV_INFO_N_ENTRIES];
	const uint8_t* value[GATT_DEV_INFO_N_ENTRIES];
};

} // namespace Gatt
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
