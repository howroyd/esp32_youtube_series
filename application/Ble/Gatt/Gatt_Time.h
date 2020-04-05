#include <ctime>

#pragma once

#include "Gatt_Base.h"

namespace Gatt
{

// --------------------------------------------------------------------------------------------------------------------
// Current Time Service
// --------------------------------------------------------------------------------------------------------------------
#define GATT_TIME_N_ENTRIES 3
class Gatt_time_svc final : public Gatt_table_base<GATT_TIME_N_ENTRIES>
{
public:
	Gatt_time_svc(const uint16_t table_id) :
			Gatt_table_base(
				esp_bt_uuid_t{
					.len = ESP_UUID_LEN_16,
					.uuid = {.uuid16 = ESP_GATT_UUID_CURRENT_TIME_SVC}
				}, table_id),
			max_length{20, 20, 20},
			length{20, 20, 20},
			value{const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("")),
					const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>("")), 
					const_cast<uint8_t*>(reinterpret_cast<const uint8_t*>(""))}
	{
		table[0] = 
		{ 
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {pri_serv_uuid.len, (uint8_t*)&pri_serv_uuid.uuid, ESP_GATT_PERM_READ, uuid_primary.len, uuid_primary.len, (uint8_t*)&uuid_primary.uuid}
		};
		table[1] = attr_entry_char_dec_read;
		table[IDX_CURRENT_TIME] = 
		{
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {uuid_list[0].len, (uint8_t*)&uuid_list[0].uuid, permissions[0], max_length[0], length[0], (uint8_t*)value[0]}
		};
		table[3] = attr_entry_char_dec_read;
		table[IDX_LOCAL_TIME_INFO] = 
		{
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {uuid_list[1].len, (uint8_t*)&uuid_list[1].uuid, permissions[1], max_length[1], length[1], (uint8_t*)value[1]}
		};
		table[5] = attr_entry_char_dec_read;
		table[IDX_REF_TIME_INFO] = 
		{
			.attr_control = {ESP_GATT_AUTO_RSP},
			.att_desc = {uuid_list[2].len, (uint8_t*)&uuid_list[2].uuid, permissions[2], max_length[2], length[2], (uint8_t*)value[2]}
		};
		table[7] = attr_entry_char_dec_read;
	}

	enum
	{
		IDX_CURRENT_TIME = 2,
		IDX_LOCAL_TIME_INFO = 4,
		IDX_REF_TIME_INFO = 6
	};

	bool update(void);

private:
	// UUIDs of characteristics within the device information service from the BLE standard
	static const esp_bt_uuid_t uuid_list[GATT_TIME_N_ENTRIES];
	static const esp_gatt_perm_t permissions[GATT_TIME_N_ENTRIES];

	const uint16_t max_length[GATT_TIME_N_ENTRIES];
	const uint16_t length[GATT_TIME_N_ENTRIES];
	const uint8_t* value[GATT_TIME_N_ENTRIES];
};

} // namespace Gatt
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
