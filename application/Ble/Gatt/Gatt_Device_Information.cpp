#include "Gatt_Device_Information.h"

namespace Gatt
{

// UUIDs of characteristics within the device information service from the BLE standard
const esp_bt_uuid_t Gatt_dev_info_svc::uuid_list[] =
{
	{.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = ESP_GATT_UUID_MANU_NAME}},
	{.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = ESP_GATT_UUID_MODEL_NUMBER_STR}},
	{.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = ESP_GATT_UUID_SERIAL_NUMBER_STR}},
	{.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = ESP_GATT_UUID_HW_VERSION_STR}},
	{.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = ESP_GATT_UUID_FW_VERSION_STR}}
};

const esp_gatt_perm_t Gatt_dev_info_svc::permissions[] =
{
	ESP_GATT_PERM_READ,
	ESP_GATT_PERM_READ,
	ESP_GATT_PERM_READ,
	ESP_GATT_PERM_READ,
	ESP_GATT_PERM_READ
};

} // namespace Gatt
