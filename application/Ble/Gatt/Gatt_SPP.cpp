#include "Gatt_SPP.h"

namespace Gatt
{

// Alcuris SPP primary UUID
const esp_bt_uuid_t Gatt_spp_svc::uuid_spp =
{
		.len = ESP_UUID_LEN_128,
		.uuid = { .uuid128 = { 0x55, 0xE4, 0x05, 0xD2, 0xAF, 0x9F, 0xA9, 0x8F, 0xE5, 0x4A, 0x7D, 0xFE, 0x43, 0x53, 0x53, 0x49 } },
};

// Alcuris write UUID for connected devices to send us data
const esp_bt_uuid_t Gatt_spp_svc::uuid_spp_write =
{
		.len = ESP_UUID_LEN_128,
		.uuid = { .uuid128 = { 0xB3, 0x9B, 0x72, 0x34, 0xBE, 0xEC, 0xD4, 0xA8, 0xF4, 0x43, 0x41, 0x88, 0x43, 0x53, 0x53, 0x49 } },
};

// Alcuris notify UUID for us to send data to a connected device
const esp_bt_uuid_t Gatt_spp_svc::uuid_spp_notify =
{
		.len = ESP_UUID_LEN_128,
		.uuid = { .uuid128 = { 0x16, 0x96, 0x24, 0x47, 0xC6, 0x23, 0x61, 0xBA, 0xD9, 0x4B, 0x4D, 0x1E, 0x43, 0x53, 0x53, 0x49 } },
};

const esp_bt_uuid_t Gatt_spp_svc::uuid_list[GATT_SPP_N_ENTRIES + GATT_SPP_N_NOTIFIES] =
{
	uuid_spp_write,
	uuid_spp_notify
};

const esp_gatt_perm_t Gatt_spp_svc::permissions[GATT_SPP_N_ENTRIES + GATT_SPP_N_NOTIFIES] =
{
	ESP_GATT_PERM_READ | ESP_GATT_PERM_WRITE,
	ESP_GATT_PERM_READ
};

const uint16_t Gatt_spp_svc::max_length[GATT_SPP_N_ENTRIES + GATT_SPP_N_NOTIFIES] =
{
	DATA_MAX_LEN,
	DATA_MAX_LEN
};

const uint16_t Gatt_spp_svc::length[GATT_SPP_N_ENTRIES + GATT_SPP_N_NOTIFIES] =
{
	DATA_MAX_LEN,
	DATA_MAX_LEN
};

} // namespace Gatt
