#include "Ble.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#define LOG_TAG "BLE"

namespace Bt_Le
{

esp_bt_mode_t Ble::mode = ESP_BT_MODE_BLE; // Our startup state

// --------------------------------------------------------------------------------------------------------------------
// GATT Services
// --------------------------------------------------------------------------------------------------------------------
// Device Information Service
Gatt::Gatt_dev_info_svc Ble::dev_info = 
{
	IDX_DEV_INFO, // Service ID
	BLE_GATTS_MANUF_NAME,
	BLE_GATTS_MODEL,
	BLE_GATTS_SERIAL_STR,
	BLE_GATTS_HW,
	BLE_GATTS_FW
};

// Alcuris SPP Service
Gatt::Gatt_spp_svc Ble::spp(IDX_SPP);

Gatt::Gatt_hub_info_svc Ble::hub_info =
{
	IDX_HUB_INFO, // Service ID
	"0",
	"GreenGiant-2G4",
	"0",
	"0"
};

Gatt::Gatt_time_svc Ble::time_info =
{
	IDX_TIME, // Service ID
};

// --------------------------------------------------------------------------------------------------------------------
// BLE Advertising
// --------------------------------------------------------------------------------------------------------------------
// Advertising RAW data (len, ID, data...)
// REF: https://www.bluetooth.com/specifications/assigned-numbers/generic-access-profile/
uint8_t Ble::adv_data_raw[] = // max 31 BYTES!!
{
    adv_data_min[0], adv_data_min[1], adv_data_min[2],

	1 + 16, gap_id_uuid, uuid[15], uuid[14], uuid[13], uuid[12], uuid[11], uuid[10], uuid[9], uuid[8],
							uuid[7], uuid[6], uuid[5], uuid[4], uuid[3], uuid[2], uuid[1], uuid[0],

    1 + 2 + 6, gap_id_name, 'G', 'G', BLE_GATTS_SERIAL_STR[0],
										BLE_GATTS_SERIAL_STR[1], 
										BLE_GATTS_SERIAL_STR[2], 
										BLE_GATTS_SERIAL_STR[3], 
										BLE_GATTS_SERIAL_STR[4], 
										BLE_GATTS_SERIAL_STR[5] // Advertising name
};

// Advertising parameters
esp_ble_adv_params_t Ble::adv_params = 
{
    .adv_int_min = 0x20,
    .adv_int_max = 0x40,
    .adv_type = ADV_TYPE_IND,
    .own_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .peer_addr = {0, 0, 0, 0, 0, 0},
    .peer_addr_type = BLE_ADDR_TYPE_PUBLIC,
    .channel_map = ADV_CHNL_ALL,
    .adv_filter_policy = ADV_FILTER_ALLOW_SCAN_ANY_CON_ANY,
};


// --------------------------------------------------------------------------------------------------------------------
// TASK
// --------------------------------------------------------------------------------------------------------------------
void Ble::task()
{
	/*
	while (start == false)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
	}*/

	ESP_LOGI(LOG_TAG, "Task running");

	// Start up BLE services
	while (init() != ESP_OK)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

	// Set our serial number to the last three bytes of the 
	//   unique ESP MAC address set in EFUSE0.
	{
		uint8_t mac_addr[6];
		if (ESP_OK == esp_efuse_mac_get_default(mac_addr))
		{
			ESP_LOGI(LOG_TAG, "Serial %02X%02X%02X", mac_addr[3], mac_addr[4], mac_addr[5]);
			change_serial_number(mac_addr);
		}
	}

	// Start advertising
	advertise();

	running = true;

	// Main task loop
	while (true)
	{
		//time_info.update();
		vTaskDelay(1000);
	}
}


// --------------------------------------------------------------------------------------------------------------------
// INTERFACE
// --------------------------------------------------------------------------------------------------------------------
// Send C style String
esp_err_t Ble::send_cstring(const char* str)
{
	// A NOTIFY has a limit of 20 characters
	esp_err_t status = ESP_OK;
	static const uint16_t max_len = 20;

	const char* p_end = str + strlen(str); // one past the end
	char* p_current = const_cast<char*>(str);

	// Loop through the string in chunks of max_len (20 bytes)
	while (p_current < p_end)
	{
		// Calculate this chunk's length
		const size_t this_length = ((strlen(p_current)) > max_len) ? max_len : (strlen(p_current));

		// BLE notify this chunk of the string
		status |= spp.notify_value(reinterpret_cast<uint8_t*>(p_current), this_length);

		// Advance to the next chunk
		p_current += this_length;

		// Delay to allow the BLE stack to write the data
		vTaskDelay(pdMS_TO_TICKS(10));
	}

    return status;
}

// --------------------------------------------------------------------------------------------------------------------
bool Ble::change_advertising_uuid(const esp_bt_uuid_t uuid)
{
	// Effectively an assert.  Currently only 128bit UUIDs supported
	//   in this code.  ESP can do 16 or 32 bit though.
	if (uuid.len != ESP_UUID_LEN_128)
	{
		ESP_LOGE(LOG_TAG, "UUID not changed (bad length)");
		return false;
	}

	const uint8_t length = uuid.len + 1;
	const uint8_t* P_uuid_end = uuid.uuid.uuid128 + uuid.len - 1;
	uint8_t* p_adv_data = adv_data_raw + sizeof(adv_data_min);

	*p_adv_data++ = length;
	*p_adv_data++ = gap_id_uuid;

	// Note; UUID needs to be big endian
	//  this loop reverses the order.
	for (size_t i = 0 ; i < uuid.len ; ++i)
	{
		*p_adv_data++ = *(P_uuid_end - i);
	}

	return true;
}

// --------------------------------------------------------------------------------------------------------------------
bool Ble::change_advertising_name(const char* new_name)
{
	const uint8_t length = strlen(new_name);

	// Effectively an assert.  Currently only 7 char stings supported
	//   in this code.  Could be less but not more if using a 128bit UUID
	//   due to advertising data max length of 31 bytes.
	if (strlen(new_name) != 8)
	{
		ESP_LOGE(LOG_TAG, "Advertising name not changed (wrong length)");
		return false;
	}

	uint8_t* p_adv_data = adv_data_raw + sizeof(adv_data_raw) - length;

	memcpy(p_adv_data, new_name, length);

	if (adv_state)
	{
		advertise(false);
		vTaskDelay(50);
		advertise();
	}

	return true;
}

// --------------------------------------------------------------------------------------------------------------------
// Method changes both advertising name and the device infomration service
bool Ble::change_serial_number(const uint32_t serial_24bit)
{
	char new_name[9];

	snprintf(new_name, sizeof(new_name), "GG%06X", serial_24bit);

	if (dev_info.change_serial(serial_24bit))
	{
		return change_advertising_name(new_name);
	}
	else
	{
		ESP_LOGE(LOG_TAG, "Serial number not changed in device information service");
		return false;
	}
}
bool Ble::change_serial_number(const uint8_t serial_24bit[6])
{
	char new_name[9];

	snprintf(new_name, sizeof(new_name), "GG%02X%02X%02X", serial_24bit[3], serial_24bit[4], serial_24bit[5]);

	if (dev_info.change_serial(serial_24bit))
	{
		return change_advertising_name(new_name);
	}
	else
	{
		ESP_LOGE(LOG_TAG, "Serial number not changed in device information service");
		return false;
	}
}


// --------------------------------------------------------------------------------------------------------------------
// BLE Advertising
// --------------------------------------------------------------------------------------------------------------------
esp_err_t Ble::advertise(const bool status)
{
	if (status)
	{
		esp_ble_gap_config_adv_data_raw((uint8_t *)adv_data_raw, sizeof(adv_data_raw));
		return esp_ble_gap_start_advertising(&adv_params);
	}
	else
	{
		return esp_ble_gap_stop_advertising();
	}
}


// --------------------------------------------------------------------------------------------------------------------
// Init
// --------------------------------------------------------------------------------------------------------------------
esp_err_t Ble::init(void)
{
	esp_err_t status = ESP_OK;

	// Initialise the dual mode Bluetooth (BLE & Classic) controller
	status |= init_common(mode);
	ESP_LOGD(LOG_TAG, "Common Init: %s", esp_err_to_name(status));

	// Register the GAP callback
	status |= esp_ble_gap_register_callback(gap_event_handler);
	ESP_LOGD(LOG_TAG, "Gap callback register: %s", esp_err_to_name(status));

	// Register the GATTS callback
	status |= esp_ble_gatts_register_callback(gatts_event_handler);
	ESP_LOGD(LOG_TAG, "Gatts callback register: %s", esp_err_to_name(status));

	// Register the app identifier
	status |= esp_ble_gatts_app_register(ESP_APP_ID); // TODO what does this do??
	ESP_LOGD(LOG_TAG, "Gap app register: %s", esp_err_to_name(status));

	if (status == ESP_OK)
	{
		// Init success, set our state
		mode = ESP_BT_MODE_BLE;

		ESP_LOGI(LOG_TAG, "Running");
	}
	else
	{
		// Init failed, clean up
		deinit();
		ESP_LOGE(LOG_TAG, "Failed to start: %s", esp_err_to_name(status));
	}

	uint16_t timeout = 10;

	while ((!dev_info.service_started() 
				&& !spp.service_started() 
				&& !hub_info.service_started()
				&& !time_info.service_started()) 
			&& timeout > 0)
	{
		vTaskDelay(pdMS_TO_TICKS(500));
		--timeout;
	}

	return status;
}


// --------------------------------------------------------------------------------------------------------------------
// Deinit
// --------------------------------------------------------------------------------------------------------------------
esp_err_t Ble::deinit(void)
{
	esp_err_t status = ESP_OK;

	ESP_LOGI(LOG_TAG, "Stopping");

	// Check if we are connected to a bonded device
	if (spp.device_connected())
	{
		// Short delay to ensure any data is pushed out before we destroy the GATTS service
		vTaskDelay(100);
		status |= spp.disconnect();
	}

	// Check if we are advertising
	if (adv_state)
	{
		// Short delay to ensure any data is pushed out before we destroy the GAP service
		vTaskDelay(100);
		status |= esp_ble_gap_stop_advertising();
	}

	// Deregister our application identifier
	status |= esp_ble_gatts_app_unregister(ESP_APP_ID);

	// Stop the GATTS service
	//status |= esp_ble_gatts_stop_service(gatt_spp_svc.tbl->id);

	// Stop and deconstruct the Bluetooth (dual mode) controller
	status |= deinit_common();

	if (status == ESP_OK)
	{
		// Deinit success, set our state
		mode = ESP_BT_MODE_IDLE;

		ESP_LOGI(LOG_TAG, "Deinit success");

		adv_state = false;
	}
	else
	{
		ESP_LOGE(LOG_TAG, "Deinit failed");
	}

	return status;
}


// --------------------------------------------------------------------------------------------------------------------
// CALLBACKS
// --------------------------------------------------------------------------------------------------------------------
void Ble::gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param)
{
    esp_err_t err;

    switch (event)
    {
    case ESP_GAP_BLE_ADV_START_COMPLETE_EVT:
        // Advertising start complete event to indicate advertising start successfully or failed
        if ((err = param->adv_start_cmpl.status) != ESP_BT_STATUS_SUCCESS)
        {
        	// Advertising start failed
			ESP_LOGE(LOG_TAG, "Advertising start failed");
        }
        else
        {
        	// Advertising start success
			ESP_LOGI(LOG_TAG, "Advertising started");
        	adv_state = true;
        }
        break;
    case ESP_GAP_BLE_ADV_STOP_COMPLETE_EVT:
    	// Advertising stop complete event to indicate advertising start successfully or failed
    	if ((err = param->adv_stop_cmpl.status) != ESP_BT_STATUS_SUCCESS)
		{
    		// Advertising stop failed
			ESP_LOGE(LOG_TAG, "Advertising stop failed");
		}
    	else
		{
    		// Advertising stop success
			adv_state = false;
			ESP_LOGI(LOG_TAG, "Advertising stopped");
		}
    	break;
    default:
    	// Unhandled
        break;
    }
}

// --------------------------------------------------------------------------------------------------------------------
void Ble::gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
    switch (event)
    {
    case ESP_GATTS_REG_EVT:
    {
        // Create our service list
		if (dev_info.create_table(gatts_if) != ESP_OK)
		{
			ESP_LOGE(LOG_TAG, "Failed to create Device Information table");
		}
		if (spp.create_table(gatts_if) != ESP_OK)
		{
			ESP_LOGE(LOG_TAG, "Failed to create SPP table");
		}
		if (hub_info.create_table(gatts_if) != ESP_OK)
		{
			ESP_LOGE(LOG_TAG, "Failed to create Hub Information table");
		}
		if (time_info.create_table(gatts_if) != ESP_OK)
		{
			ESP_LOGE(LOG_TAG, "Failed to create Time table");
		}
        break;
    }
    case ESP_GATTS_READ_EVT:
    	// Not used at the moment but may be required so leaving code in
        break;
    case ESP_GATTS_WRITE_EVT:
    {
    	// Someone has sent us some data

        // Check that we've actually received data and this isn't a "prepare to receive" command
        if (param->write.is_prep == false)
        {
#if 0
            if (res == gatt_spp_svc.DATA_NTF_CFG)
            {
            	// Notification configuration data.  Currently unused.
                if ((param->write.len == 2) && (param->write.value[0] == 0x01) && (param->write.value[1] == 0x00))
                {
                    enable_data_ntf = true;
                }
                else if ((param->write.len == 2) && (param->write.value[0] == 0x00) && (param->write.value[1] == 0x00))
                {
                    enable_data_ntf = false;
                }
            }
#endif				
            if (param->write.handle == spp[spp.IDX_WRITE_VAL] && param->write.len > 0)
            {
				char buf[100]{};
				memcpy(buf, param->write.value, param->write.len);
				ESP_LOGD(LOG_TAG, "Data received: \"%s\"", buf);
            }
        }
		else
		{
			ESP_LOGW(LOG_TAG, "Someone tried to prepare us for writing SPP data. Unimplemented!");
		}
		
        break;
    }
    case ESP_GATTS_CONNECT_EVT:
    {
    	// A device has connected to us
		ESP_LOGI(LOG_TAG, "Device connected id=%u", param->connect.conn_id);

		// Set our connection identifiers
		spp.save_connection_info(param->connect.conn_id, gatts_if, param->connect.remote_bda);

        // Stop advertising
        advertise(false);

        break;
    }
    case ESP_GATTS_DISCONNECT_EVT:
    {
    	// A connected device has disconnected
		ESP_LOGI(LOG_TAG, "Device disconnected id=%u", param->disconnect.conn_id);

    	spp.clear_connection_info();
        enable_data_ntf = false;

        // Re-start advertising*/
        advertise();

        break;
    }
    case ESP_GATTS_CREAT_ATTR_TAB_EVT:
    {
    	// GATTS service created
    	if (param->add_attr_tab.status == ESP_GATT_OK)
    	{
    		if (param->add_attr_tab.svc_inst_id == dev_info.id && param->add_attr_tab.num_handle == dev_info.n_entries)
    		{
    			if (dev_info.start_service(param->add_attr_tab.handles) != ESP_OK)
				{
					ESP_LOGE(LOG_TAG, "Failed to start device information service");
				}
				else
				{
					ESP_LOGI(LOG_TAG, "Device information service started");
				}
    		}
    		else if (param->add_attr_tab.svc_inst_id == spp.id && param->add_attr_tab.num_handle == spp.n_entries)
    		{
    			if (spp.start_service(param->add_attr_tab.handles) != ESP_OK)
				{
					ESP_LOGE(LOG_TAG, "Failed to start SPP service");
				}
				else
				{
					ESP_LOGI(LOG_TAG, "SPP started");
				}
    		}
    		else if (param->add_attr_tab.svc_inst_id == hub_info.id && param->add_attr_tab.num_handle == hub_info.n_entries)
			{
				if (hub_info.start_service(param->add_attr_tab.handles) != ESP_OK)
				{
					ESP_LOGE(LOG_TAG, "Failed to start Hub Information service");
				}
				else
				{
					ESP_LOGI(LOG_TAG, "Hub Information service started");
				}
			}
			else if (param->add_attr_tab.svc_inst_id == time_info.id && param->add_attr_tab.num_handle == time_info.n_entries)
			{
				if (time_info.start_service(param->add_attr_tab.handles) != ESP_OK)
				{
					ESP_LOGE(LOG_TAG, "Failed to start Time service");
				}
				else
				{
					ESP_LOGI(LOG_TAG, "Time service started");
				}
			}
    	}

        break;
    }
	case ESP_GATTS_SET_ATTR_VAL_EVT:
		if (param->set_attr_val.status != ESP_OK)
		{
			ESP_LOGE(LOG_TAG, "Attribute %u not changed for service %u", param->set_attr_val.attr_handle, param->set_attr_val.srvc_handle);
		}
		break;
	case ESP_GATTS_CONGEST_EVT:
		ESP_LOGE(LOG_TAG, "Congestion!");
		break;
    case ESP_GATTS_CREATE_EVT:
		if (param->create.status == ESP_OK)
		{
			if (param->create.service_handle == dev_info[0])
			{
				;
			}
			else if (param->create.service_handle == spp[0])
			{
				;
			}
			else if (param->create.service_handle == time_info[0])
			{
				;
			}
		}
		break;
	default:
        break;
    }
}

// --------------------------------------------------------------------------------------------------------------------
void Ble::gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param)
{
	if (event == ESP_GATTS_REG_EVT)
	{
		if (param->reg.status == ESP_GATT_OK)
		{
			profile_tab[IDX_SPP].gatts_if = gatts_if;
		}
		else
		{
			return;
		}
	}

    // If event is register event, store the gatts_if for each profile
	for (size_t idx = 0; idx < IDX_N_IDX_ENTRIES; ++idx)
	{
		if (gatts_if == ESP_GATT_IF_NONE || // ESP_GATT_IF_NONE, not specify a certain gatt_if, need to call every profile cb function
			gatts_if == profile_tab[idx].gatts_if)
		{
			if (profile_tab[idx].gatts_cb)
			{
				profile_tab[idx].gatts_cb(event, gatts_if, param);
			}
		}
	}
}


// --------------------------------------------------------------------------------------------------------------------
// ESP32 BLE Statics
// --------------------------------------------------------------------------------------------------------------------
bool Ble::adv_state = false;
bool Ble::enable_data_ntf = false;

//uint16_t Ble::spp_s::mtu_size = 23;

struct Ble::gatts_profile_inst Ble::profile_tab[IDX_N_IDX_ENTRIES] = 
{
	[IDX_DEV_INFO] =
	{
		.gatts_cb = gatts_profile_event_handler,
		.gatts_if = ESP_GATT_IF_NONE // Not get the gatt_if, so initial is ESP_GATT_IF_NONE
	},
	[IDX_SPP] =
	{
		.gatts_cb = gatts_profile_event_handler,
		.gatts_if = ESP_GATT_IF_NONE // Not get the gatt_if, so initial is ESP_GATT_IF_NONE
	},
	[IDX_HUB_INFO] =
	{
		.gatts_cb = gatts_profile_event_handler,
		.gatts_if = ESP_GATT_IF_NONE // Not get the gatt_if, so initial is ESP_GATT_IF_NONE
	},
	[IDX_TIME] =
	{
		.gatts_cb = gatts_profile_event_handler,
		.gatts_if = ESP_GATT_IF_NONE // Not get the gatt_if, so initial is ESP_GATT_IF_NONE
	}
};

} // namespace Bt_Le
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------