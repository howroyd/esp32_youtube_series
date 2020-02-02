#pragma once

#include "Gatt_Base.h"

namespace Gatt
{

// --------------------------------------------------------------------------------------------------------------------
// Device Information Service
// --------------------------------------------------------------------------------------------------------------------
#define GATT_HUB_INFO_N_ENTRIES 1
#define GATT_HUB_INFO_N_NOTIFIES 3
#define HUB_INFO_DATA_MAX_LEN 20
class Gatt_hub_info_svc final : public Gatt_table_base<GATT_HUB_INFO_N_ENTRIES, GATT_HUB_INFO_N_NOTIFIES>
{
	// Alcuris Hub Information primary UUID
	static const esp_bt_uuid_t uuid_hub_info;

	static const esp_bt_uuid_t uuid_paired_status;
	static const esp_bt_uuid_t uuid_ssid;
	static const esp_bt_uuid_t uuid_wfi_status;
	static const esp_bt_uuid_t uuid_cell_status;

	const char* const paired_status;
	const char* const ssid;
	const char* const wifi_status;
	const char* const cell_status;

public:
	Gatt_hub_info_svc(const uint16_t table_id,
						const char* const paired_status,
						const char* const ssid,
						const char* const wifi_status,
						const char* const cell_status) :
			Gatt_table_base(uuid_hub_info, table_id),
			paired_status(paired_status),
			ssid(ssid),
			wifi_status(wifi_status),
			cell_status(cell_status),
			length{static_cast<uint16_t>(strlen(ssid))}
	{
		table[0] = 
		{ 
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {pri_serv_uuid.len, (uint8_t*)&pri_serv_uuid.uuid, ESP_GATT_PERM_READ, uuid_primary.len, uuid_primary.len, (uint8_t*)&uuid_primary.uuid}
		};
		table[1] = attr_entry_char_dec_read_notify;
		table[IDX_PAIRED_VAL] =
		{
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {uuid_list[0].len, (uint8_t*)&uuid_list[0].uuid, ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), (uint8_t*)paired_status}
		};
		table[3] =
		{
			.attr_control = {ESP_GATT_AUTO_RSP},
			.att_desc = {char_client_conf_uuid.len, (uint8_t*)&char_client_conf_uuid.uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(notify_paired_ccc), (uint8_t*)notify_paired_ccc}
		};
		table[4] = attr_entry_char_dec_read_notify;
		table[IDX_WIFI_VAL] =
		{
			.attr_control = {ESP_GATT_AUTO_RSP},
			.att_desc = {uuid_list[1].len, (uint8_t*)&uuid_list[1].uuid, ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), (uint8_t*)wifi_status}
		};
		table[6] =
		{
			.attr_control = {ESP_GATT_AUTO_RSP},
			.att_desc = {char_client_conf_uuid.len, (uint8_t*)&char_client_conf_uuid.uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(notify_wifi_ccc), (uint8_t*)notify_wifi_ccc}
		};
		table[7] = attr_entry_char_dec_read_notify;
		table[IDX_CELL_VAL] =
		{
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {uuid_list[2].len, (uint8_t *)&uuid_list[2].uuid, ESP_GATT_PERM_READ, sizeof(uint8_t), sizeof(uint8_t), (uint8_t*)cell_status}
		};
		table[9] =
		{
			.attr_control = {ESP_GATT_AUTO_RSP},
			.att_desc = {char_client_conf_uuid.len, (uint8_t*)&char_client_conf_uuid.uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(notify_cell_ccc), (uint8_t*)notify_cell_ccc}
		};
		table[10] = attr_entry_char_dec_read;
		table[IDX_SSID_VAL] =
		{
			.attr_control = {ESP_GATT_AUTO_RSP},
			.att_desc = {uuid_list[3].len, (uint8_t*)&uuid_list[3].uuid, ESP_GATT_PERM_READ, max_length, length[0], (uint8_t*)ssid}
		};
	}

	enum
	{
		IDX_PAIRED_VAL = 2,
		IDX_WIFI_VAL = 5,
		IDX_CELL_VAL = 8,
		IDX_SSID_VAL = 11
	};

private:
	// UUIDs of characteristics within the device information service from the BLE standard
	static const esp_bt_uuid_t uuid_list[GATT_HUB_INFO_N_ENTRIES + GATT_HUB_INFO_N_NOTIFIES];

	const uint8_t value_ssid[GATT_HUB_INFO_N_ENTRIES][HUB_INFO_DATA_MAX_LEN] = {{0}};
	const uint16_t max_length = HUB_INFO_DATA_MAX_LEN;
	const uint16_t length[GATT_HUB_INFO_N_ENTRIES];

	const uint8_t notify_paired_ccc[2] = {0x00, 0x00};
	const uint8_t notify_wifi_ccc[2] = {0x00, 0x00};
	const uint8_t notify_cell_ccc[2] = {0x00, 0x00};
};

} // namespace Gatt
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
