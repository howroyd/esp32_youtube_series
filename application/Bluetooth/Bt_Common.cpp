#include "Bt_Common.h"

namespace Bt
{

esp_bt_mode_t BtCommon::mode = ESP_BT_MODE_IDLE;


// --------------------------------------------------------------------------------------------------------------------
// Interface
// --------------------------------------------------------------------------------------------------------------------
esp_err_t BtCommon::init_common(esp_bt_mode_t mode)
{
	// Effectively a semaphore. Ensures two cores/tasks aren't calling this method simultaneously
    while (init_called == init_e::WAIT)
    {
    	vTaskDelay(10);
    }

    // Check if we are already init'd
    if (BtCommon::init_called == init_e::DONE)
    {
    	return init_success ? ESP_OK : ESP_FAIL;
    }

    // Set our state so nobody else tries to run this method on another core/task
    init_called = init_e::WAIT;

    // Set our Bluetooth controller mode
    BtCommon::mode = mode;

    // Initialize NVS as it is used to store PHY calibration data
    esp_err_t ret = nvs_flash_init();

    // Check the NVS is valid
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ret = nvs_flash_init();
    }

/*
	// Release memory if not using dual mode Bluetooth
	// Note; this is a one way trip until reboot.  Cannot be undone.
    switch (mode)
    {
    case ESP_BT_MODE_BLE:
        printf("Releasing memory for the unused BT Classic controller\r\n");
        ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_CLASSIC_BT));
        break;
    case ESP_BT_MODE_CLASSIC_BT:
        printf("Releasing memory for the unused BLE controller\r\n");
        ESP_ERROR_CHECK(esp_bt_controller_mem_release(ESP_BT_MODE_BLE));
        break;
    default:
        break;
    }
*/

    // Get our init parameters for our Bluetooth controller from our menuconfig settings
    esp_bt_controller_config_t bt_cfg = BT_CONTROLLER_INIT_CONFIG_DEFAULT();

    // Initialise the Bluetooth controller
    if ((ret = esp_bt_controller_init(&bt_cfg)) != ESP_OK)
    {
        return ret;
    }

    // Enable the Bluetooth controller
    if ((ret = esp_bt_controller_enable(ESP_BT_MODE_BTDM)) != ESP_OK)
    {
        return ret;
    }

    // Init our Bluetooth stack
    if ((ret = esp_bluedroid_init()) != ESP_OK)
    {
        return ret;
    }

    // Enable our Bluetooth stack
    if ((ret = esp_bluedroid_enable()) != ESP_OK)
    {
        return ret;
    }

    // If we get here, success!
    init_success = true; // We're init'd
    init_called = init_e::DONE; // Give our psuedo semaphore

    return ESP_OK;
}

// --------------------------------------------------------------------------------------------------------------------
esp_err_t BtCommon::deinit_common(void)
{
	// Effectively a semaphore. Ensures two cores/tasks aren't calling this method simultaneously
	while (init_called == init_e::WAIT)
	{
		vTaskDelay(100);
	}

    // Check if we are already deinit'd
    if (BtCommon::init_called == init_e::GO)
    {
    	return init_success ? ESP_FAIL : ESP_OK;
    }

    // Set our state so nobody else tries to run this method on another core/task
    init_called = init_e::WAIT;


	// Disable then deinit the stack and the Bluetooth controller
	esp_bluedroid_disable();
	esp_bluedroid_deinit();
	esp_bt_controller_disable();
	esp_bt_controller_deinit();

	// If we get here, success!
	init_success = false; // We're deinit'd
	init_called = init_e::GO; // Give our psuedo semaphore

	return ESP_OK;
}


// --------------------------------------------------------------------------------------------------------------------
// Helper functions
// --------------------------------------------------------------------------------------------------------------------
// https://github.com/espressif/esp-idf/blob/release/v4.0/examples/bluetooth/bluedroid/classic_bt/bt_discovery/main/bt_discovery.c
char* BtCommon::bda2str(const esp_bd_addr_t bda, char *str, const size_t size)
{
    if (bda == nullptr || str == nullptr || size < 18)
    {
        return nullptr;
    }

    snprintf(str, size, "%02X:%02X:%02X:%02X:%02X:%02X",
    		bda[0], bda[1], bda[2], bda[3], bda[4], bda[5]);

    return str;
}

// --------------------------------------------------------------------------------------------------------------------
char* BtCommon::uuid2str(esp_bt_uuid_t* uuid, char* str, size_t size)
{
    if (uuid == nullptr || str == nullptr)
    {
        return nullptr;
    }

    if (uuid->len == 2 && size >= 5)
    {
        snprintf(str, size, "%04X", uuid->uuid.uuid16);
    }
    else if (uuid->len == 4 && size >= 9)
    {
        snprintf(str, size, "%08X", uuid->uuid.uuid32);
    }
    else if (uuid->len == 16 && size >= 37)
    {
    	// Note; 128 UUID is big endian so this flips it to little endian.
        const uint8_t* p = uuid->uuid.uuid128;
        snprintf(str, size,
        		"%02X%02X%02X%02X-%02X%02X-%02X%02X-%02X%02X-%02X%02X%02X%02X%02X%02X",
                p[15], p[14], p[13], p[12], p[11], p[10], p[9], p[8],
                p[7],  p[6],  p[5],  p[4],  p[3],  p[2],  p[1], p[0]);
    }
    else
    {
        return nullptr;
    }

    return str;
}


// --------------------------------------------------------------------------------------------------------------------
// ESP32 BT Common Statics
// --------------------------------------------------------------------------------------------------------------------
BtCommon::init_e BtCommon::init_called = init_e::GO;
bool BtCommon::init_success = false;

} //namespace Bt
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
