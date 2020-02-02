#include "Bt_Classic.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#define LOG_TAG "BT_CLASSIC"

namespace Bt_Classic
{

esp_bt_mode_t BtClassic::mode = ESP_BT_MODE_CLASSIC_BT; // Our startup state

// --------------------------------------------------------------------------------------------------------------------
// TASK
// --------------------------------------------------------------------------------------------------------------------
void BtClassic::task()
{
    ESP_LOGI(LOG_TAG, "Task running");

    // Start up BT Classic services
	while (init() != ESP_OK)
	{
		vTaskDelay(pdMS_TO_TICKS(1000));
	}

    // BLOCKER - -END OF CODE TIDY UP
    while (true)
    {
        ESP_LOGW(LOG_TAG, "Blocker! Stack free (min) %u bytes", uxTaskGetStackHighWaterMark(NULL));
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }

    bt_app_gap_start_up();

    // Wait here until we have a valid device
    while (m_dev_info.state != APP_GAP_STATE_SERVICE_DISCOVER_COMPLETE)
    {
        vTaskDelay(1000);
    }

    ESP_LOGI(LOG_TAG, "Device found and saved!");

    esp_a2d_source_init();

    esp_a2d_source_connect(m_dev_info.bda);

    ESP_LOGI(LOG_TAG, "Task ended!");

    while (true) // TODO does this need to be here or can this task be destroyed?
    {
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
}


// --------------------------------------------------------------------------------------------------------------------
// BT Classic Advertising
// --------------------------------------------------------------------------------------------------------------------
esp_err_t BtClassic::advertise(const bool status)
{
	if (status)
	{
		return esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
	}
	else
	{
		return esp_bt_gap_set_scan_mode(ESP_BT_NON_CONNECTABLE, ESP_BT_NON_DISCOVERABLE);
	}
}


// --------------------------------------------------------------------------------------------------------------------
// Init
// --------------------------------------------------------------------------------------------------------------------
esp_err_t BtClassic::init(void)
{
	esp_err_t status = ESP_OK;

	// Initialise the dual mode Bluetooth (BLE & Classic) controller
	status |= init_common(mode);

	// Register GAP callback
	status |= esp_bt_gap_register_callback(bt_app_gap_cb);

	// Set device name
	uint8_t mac_addr[8];
	if (ESP_OK == esp_efuse_mac_get_default(mac_addr))
	{
		char new_name[9];
		snprintf(new_name, sizeof(new_name), "mh%02X%02X%02X", mac_addr[3], mac_addr[4], mac_addr[5]);
		esp_bt_dev_set_device_name(new_name);
	}
	else
	{
		esp_bt_dev_set_device_name("mhXXXXXX");
	}

	// Register the SPP callback
	status |= esp_spp_register_callback(esp_spp_cb);

	// Initialise SPP
	status |= esp_spp_init(esp_spp_mode);

	if (status == ESP_OK)
	{
		// Init success, set our state and inform the H7
		mode = ESP_BT_MODE_CLASSIC_BT;

		// Inform the H7 that we are fully running
		ESP_LOGI(LOG_TAG, "Init OK");
	}
	else
	{
		// Init failed, clean up and inform H7
		deinit();
		ESP_LOGE(LOG_TAG, "Failed to start: %s", esp_err_to_name(status));
	}

	return status;
}


// --------------------------------------------------------------------------------------------------------------------
// Deinit
// --------------------------------------------------------------------------------------------------------------------
esp_err_t BtClassic::deinit(void)
{
	esp_err_t status = ESP_OK;

	ESP_LOGI(LOG_TAG, "Stopping");

	// Deregister SPP
	status |= esp_spp_deinit();

	// Stop and deconstruct the Bluetooth (dual mode) controller
	status |= deinit_common();

	if (status == ESP_OK)
	{
		// Deinit success, set our state and inform the H7
		mode = ESP_BT_MODE_IDLE;

		ESP_LOGI(LOG_TAG, "Stopped");
	}
	else
	{
		ESP_LOGE(LOG_TAG, "Failed to deinit");
	}

	return status;
}

bool BtClassic::get_name_from_eir(uint8_t *eir, uint8_t *bdname, uint8_t *bdname_len)
{
    uint8_t *rmt_bdname = NULL;
    uint8_t rmt_bdname_len = 0;

    if (!eir)
    {
        return false;
    }

    rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_CMPL_LOCAL_NAME, &rmt_bdname_len);
    if (!rmt_bdname)
    {
        rmt_bdname = esp_bt_gap_resolve_eir_data(eir, ESP_BT_EIR_TYPE_SHORT_LOCAL_NAME, &rmt_bdname_len);
    }

    if (rmt_bdname)
    {
        if (rmt_bdname_len > ESP_BT_GAP_MAX_BDNAME_LEN)
        {
            rmt_bdname_len = ESP_BT_GAP_MAX_BDNAME_LEN;
        }

        if (bdname)
        {
            memcpy(bdname, rmt_bdname, rmt_bdname_len);
            bdname[rmt_bdname_len] = '\0';
        }
        if (bdname_len)
        {
            *bdname_len = rmt_bdname_len;
        }
        return true;
    }

    return false;
}

void BtClassic::update_device_info(esp_bt_gap_cb_param_t *param)
{
    char bda_str[18];
    uint32_t cod = 0;
    int32_t rssi = -129; // invalid value
    esp_bt_gap_dev_prop_t *p;

    ESP_LOGI(LOG_TAG, "Device found: %s", bda2str(param->disc_res.bda, bda_str, 18));

    for (int i = 0; i < param->disc_res.num_prop; i++)
    {
        p = param->disc_res.prop + i;
        switch (p->type)
        {
        case ESP_BT_GAP_DEV_PROP_COD:
            cod = *(uint32_t *)(p->val);
            ESP_LOGI(LOG_TAG, "--Class of Device: 0x%x", cod);
            break;
        case ESP_BT_GAP_DEV_PROP_RSSI:
            rssi = *(int8_t *)(p->val);
            ESP_LOGI(LOG_TAG, "--RSSI: 0x%d", rssi);
            break;
        case ESP_BT_GAP_DEV_PROP_BDNAME:
        default:
            break;
        }
    }

    // search for device with MAJOR service class as "rendering" in COD
    app_gap_cb_t *p_dev = &m_dev_info;
    if (p_dev->dev_found && 0 != memcmp(param->disc_res.bda, p_dev->bda, ESP_BD_ADDR_LEN))
    {
        return;
    }

    if (!esp_bt_gap_is_valid_cod(cod) ||
        !(esp_bt_gap_get_cod_major_dev(cod) == ESP_BT_COD_MAJOR_DEV_AV))
    {
        return;
    }

    memcpy(p_dev->bda, param->disc_res.bda, ESP_BD_ADDR_LEN);
    p_dev->dev_found = true;
    for (int i = 0; i < param->disc_res.num_prop; i++)
    {
        p = param->disc_res.prop + i;
        switch (p->type)
        {
        case ESP_BT_GAP_DEV_PROP_COD:
            p_dev->cod = *(uint32_t *)(p->val);
            break;
        case ESP_BT_GAP_DEV_PROP_RSSI:
            p_dev->rssi = *(int8_t *)(p->val);
            break;
        case ESP_BT_GAP_DEV_PROP_BDNAME:
        {
            uint8_t len = (p->len > ESP_BT_GAP_MAX_BDNAME_LEN) ? ESP_BT_GAP_MAX_BDNAME_LEN : (uint8_t)p->len;
            memcpy(p_dev->bdname, (uint8_t *)(p->val), len);
            p_dev->bdname[len] = '\0';
            p_dev->bdname_len = len;
            break;
        }
        case ESP_BT_GAP_DEV_PROP_EIR:
        {
            memcpy(p_dev->eir, (uint8_t *)(p->val), p->len);
            p_dev->eir_len = p->len;
            break;
        }
        default:
            break;
        }
    }

    if (p_dev->eir && p_dev->bdname_len == 0)
    {
        get_name_from_eir(p_dev->eir, p_dev->bdname, &p_dev->bdname_len);
        ESP_LOGI(LOG_TAG, "Found a target device, address %s, name %s", bda_str, p_dev->bdname);
        p_dev->state = APP_GAP_STATE_DEVICE_DISCOVER_COMPLETE;
        esp_bt_gap_cancel_discovery();
    }
}

void BtClassic::bt_app_gap_init(void)
{
    app_gap_cb_t *p_dev = &m_dev_info;
    memset(p_dev, 0, sizeof(app_gap_cb_t));
}

void BtClassic::bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param)
{
    app_gap_cb_t *p_dev = &m_dev_info;
    char bda_str[18];
    char uuid_str[37];

    switch (event)
    {
    case ESP_BT_GAP_DISC_RES_EVT:
    {
        update_device_info(param);
        break;
    }
    case ESP_BT_GAP_DISC_STATE_CHANGED_EVT:
    {
        if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STOPPED)
        {
            ESP_LOGI(LOG_TAG, "Device discovery stopped");
            if ((p_dev->state == APP_GAP_STATE_DEVICE_DISCOVER_COMPLETE ||
                 p_dev->state == APP_GAP_STATE_DEVICE_DISCOVERING) &&
                p_dev->dev_found)
            {
                p_dev->state = APP_GAP_STATE_SERVICE_DISCOVERING;
                ESP_LOGI(LOG_TAG, "Discover services...");
                esp_bt_gap_get_remote_services(p_dev->bda);
            }
        }
        else if (param->disc_st_chg.state == ESP_BT_GAP_DISCOVERY_STARTED)
        {
            ESP_LOGI(LOG_TAG, "Discovery started");
        }
        break;
    }
    case ESP_BT_GAP_RMT_SRVCS_EVT:
    {
        if (memcmp(param->rmt_srvcs.bda, p_dev->bda, ESP_BD_ADDR_LEN) == 0 &&
            p_dev->state == APP_GAP_STATE_SERVICE_DISCOVERING)
        {
            p_dev->state = APP_GAP_STATE_SERVICE_DISCOVER_COMPLETE;
            if (param->rmt_srvcs.stat == ESP_BT_STATUS_SUCCESS)
            {
                ESP_LOGI(LOG_TAG, "Services for device %s found", bda2str(p_dev->bda, bda_str, 18));
                for (int i = 0; i < param->rmt_srvcs.num_uuids; i++)
                {
                    esp_bt_uuid_t *u = param->rmt_srvcs.uuid_list + i;
                    
                    ESP_LOGI(LOG_TAG, "UUID %s", uuid2str(u, uuid_str, 37));
                }
            }
            else
            {
                ESP_LOGW(LOG_TAG, "Services for device %s not found", bda2str(p_dev->bda, bda_str, 18));
            }
        }
        break;
    }
    case ESP_BT_GAP_RMT_SRVC_REC_EVT:
    default:
    {
        ESP_LOGD(LOG_TAG, "Unhandled event: %d", event);
        break;
    }
    }
    return;
}

void BtClassic::bt_app_gap_start_up(void)
{
    advertise(); // We probably don't want to do this?

    /* inititialize device information and status */
    app_gap_cb_t *p_dev = &m_dev_info;
    memset(p_dev, 0, sizeof(app_gap_cb_t));

    /* start to discover nearby Bluetooth devices */
    p_dev->state = APP_GAP_STATE_DEVICE_DISCOVERING;
    esp_bt_gap_start_discovery(ESP_BT_INQ_MODE_GENERAL_INQUIRY, 10, 0);
}


// --------------------------------------------------------------------------------------------------------------------
// CALLBACKS
// --------------------------------------------------------------------------------------------------------------------
void BtClassic::esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param)
{
    switch (event)
    {
    case ESP_SPP_INIT_EVT:
    {
    	uint8_t mac_addr[8];
    	if (ESP_OK == esp_efuse_mac_get_default(mac_addr))
    	{
    		char new_name[9];
    		snprintf(new_name, sizeof(new_name), "MH%02X%02X%02X", mac_addr[3], mac_addr[4], mac_addr[5]);
    		esp_bt_dev_set_device_name(new_name);
		}
    	else
    	{
    		esp_bt_dev_set_device_name("MHXXXXXX");
    	}
        esp_bt_gap_set_scan_mode(ESP_BT_CONNECTABLE, ESP_BT_GENERAL_DISCOVERABLE);
        esp_spp_start_srv(sec_mask, role_slave, 0, "SPP");
        break;
    }
    case ESP_SPP_DATA_IND_EVT:
        break;
    default:
        break;
    }
}


// --------------------------------------------------------------------------------------------------------------------
// ESP32 BLE Statics
// --------------------------------------------------------------------------------------------------------------------
BtClassic::app_gap_cb_t BtClassic::m_dev_info;

const esp_spp_mode_t BtClassic::esp_spp_mode = ESP_SPP_MODE_CB;
const esp_spp_sec_t BtClassic::sec_mask = ESP_SPP_SEC_AUTHENTICATE;
const esp_spp_role_t BtClassic::role_slave = ESP_SPP_ROLE_SLAVE;
long BtClassic::data_num = 0;

} // namespace Bt_Classic
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
