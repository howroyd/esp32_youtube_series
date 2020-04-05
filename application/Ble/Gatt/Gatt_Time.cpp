#include "Gatt_Time.h"

namespace Gatt
{

// UUIDs of characteristics within the device information service from the BLE standard
const esp_bt_uuid_t Gatt_time_svc::uuid_list[] =
{
	{.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = ESP_GATT_UUID_CURRENT_TIME}},
	{.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = ESP_GATT_UUID_LOCAL_TIME_INFO}},
	{.len = ESP_UUID_LEN_16, .uuid = {.uuid16 = ESP_GATT_UUID_REF_TIME_INFO}}
};

const esp_gatt_perm_t Gatt_time_svc::permissions[] =
{
	ESP_GATT_PERM_READ,
	ESP_GATT_PERM_READ,
	ESP_GATT_PERM_READ
};

bool Gatt_time_svc::update(void)
{
	std::time_t result = std::time(nullptr);
	const char* now = std::asctime(std::localtime(&result));

	//value[IDX_CURRENT_TIME] = now;
	//length[IDX_CURRENT_TIME] = strlen(now);

	return ESP_OK == update_value(IDX_CURRENT_TIME, (uint8_t*)now, strlen(now));
}

} // namespace Gatt
