#pragma once

#include "Gatt_Base.h"

namespace Gatt
{

// --------------------------------------------------------------------------------------------------------------------
// Alcuris Serial Port Profile Service
// --------------------------------------------------------------------------------------------------------------------
#define GATT_SPP_N_ENTRIES 1
#define GATT_SPP_N_NOTIFIES 1

class Gatt_spp_svc final : public Gatt_table_base<GATT_SPP_N_ENTRIES, GATT_SPP_N_NOTIFIES>
{
	// Alcuris SPP primary UUID
	static const esp_bt_uuid_t uuid_spp;

	// Alcuris write UUID for connected devices to send us data
	static const esp_bt_uuid_t uuid_spp_write;

	// Alcuris notify UUID for us to send data to a connected device
	static const esp_bt_uuid_t uuid_spp_notify;

public:
	Gatt_spp_svc(const uint16_t table_id) :
			Gatt_table_base(uuid_spp, table_id),
			data_write{},
			data_notify{},
			value{ reinterpret_cast<const uint8_t*>(data_write), 
					reinterpret_cast<const uint8_t*>(data_notify)}
	{
		table[0] = 
		{ 
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {pri_serv_uuid.len, (uint8_t*)&pri_serv_uuid.uuid, ESP_GATT_PERM_READ, uuid_primary.len, uuid_primary.len, (uint8_t*)&uuid_primary.uuid}
		};
		table[1] = attr_entry_char_dec_read_write;
		table[IDX_WRITE_VAL] = 
		{
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {uuid_list[0].len, (uint8_t*)&uuid_list[0].uuid, permissions[0], max_length[0], length[0], (uint8_t*)&value[0]}
		};
		table[3] = attr_entry_char_dec_read_notify;
		table[IDX_NOTIFY_VAL] = 
		{
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {uuid_list[1].len, (uint8_t*)&uuid_list[1].uuid, permissions[1], max_length[1], length[1], (uint8_t*)&value[1]}
		};
		table[5] = 
		{
			.attr_control = {ESP_GATT_AUTO_RSP}, 
			.att_desc = {char_client_conf_uuid.len, (uint8_t*)&char_client_conf_uuid.uuid, ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE, sizeof(uint16_t), sizeof(notify_ccc), (uint8_t*)notify_ccc}
		};
	}

	enum
	{
		IDX_WRITE_VAL = 2,
		IDX_NOTIFY_VAL = 4
	};

	// Method to save a connected devices connection information
	bool save_connection_info(const uint16_t conn_id, 
								const esp_gatt_if_t gatts_if, 
								const esp_bd_addr_t remote_bda)
	{
		this->conn_id = conn_id;
		this->gatts_if = gatts_if;
		memcpy(&this->remote_bda, &remote_bda, sizeof(esp_bd_addr_t));
		connected = true;

		return connected;
	};

	// Method to clear a connected devices connection information when it disconnects
	bool clear_connection_info(void)
	{
		conn_id = 0xFFFF;
		gatts_if = ESP_GATT_IF_NONE;
		memset(&remote_bda, 0xFF, sizeof(esp_bd_addr_t));
		connected = false;

		return connected;
	}

	// Method to send data to a connected device via the notification characteristic
	esp_err_t notify_value(const uint8_t* buf, const uint16_t buf_len)
	{
		return esp_ble_gatts_send_indicate(gatts_if, conn_id, handle_table[IDX_NOTIFY_VAL],
												buf_len, const_cast<uint8_t*>(buf), false);
	}

	// Method to return if a device is connected
	bool device_connected(void)
	{
		return connected;
	}

	// Method to disconnect from a connected device
	esp_err_t disconnect(void)
	{
		esp_err_t status = esp_ble_gatts_close(gatts_if, conn_id);

		if (status == ESP_OK)
		{
			clear_connection_info();
		}

		return status;
	}

protected:
	bool connected = false;
	uint16_t conn_id = 0xFFFF;
	esp_gatt_if_t gatts_if = ESP_GATT_IF_NONE;
	esp_bd_addr_t remote_bda = {0xFF};

private:
	static const esp_bt_uuid_t uuid_list[GATT_SPP_N_ENTRIES + GATT_SPP_N_NOTIFIES];

	static const esp_gatt_perm_t permissions[GATT_SPP_N_ENTRIES + GATT_SPP_N_NOTIFIES];

	static const uint16_t max_length[GATT_SPP_N_ENTRIES + GATT_SPP_N_NOTIFIES];
	static const uint16_t length[GATT_SPP_N_ENTRIES + GATT_SPP_N_NOTIFIES];

	const char data_write[DATA_MAX_LEN];
	const char data_notify[DATA_MAX_LEN];
	
	const uint8_t* value[GATT_SPP_N_ENTRIES + GATT_SPP_N_NOTIFIES];
	const uint8_t notify_ccc[2] = {0x00, 0x00};
};

} // namespace Gatt
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
