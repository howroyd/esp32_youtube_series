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

bool Gatt_time_svc::update(void)
{
	esp_err_t status{ESP_OK};

	status |= update_current_time();
	status |= update_local_time_info();

	return status;
}

esp_err_t Gatt_time_svc::update_current_time(void)
{
	const std::time_t result = std::time(nullptr);
	const std::tm* local = std::localtime(&result);
	//const char* now = std::asctime(local);

	uint8_t buf[10]{};
	*(uint16_t*)&buf[0] = local->tm_year + 1900;
	buf[2] = local->tm_mon + 1;
	buf[3] = local->tm_mday;
	buf[4] = local->tm_hour;
	buf[5] = local->tm_min;
	buf[6] = local->tm_sec;
	buf[7] = local->tm_wday == 0 ? 7 : local->tm_wday;

	//value[IDX_CURRENT_TIME] = now;
	//length[IDX_CURRENT_TIME] = strlen(now);

	//return ESP_OK == update_value(IDX_CURRENT_TIME, (uint8_t*)now, strlen(now));
	return ESP_OK == update_value(IDX_CURRENT_TIME, buf, sizeof(buf));
}

esp_err_t Gatt_time_svc::update_local_time_info(void)
{
	const std::time_t result = std::time(nullptr);
	const std::tm* local = std::localtime(&result);

	int8_t timezone{0};
	uint8_t dst{static_cast<uint8_t>(local->tm_isdst)};

	uint8_t buf[2]{};
	buf[0] = timezone;
	buf[1] = dst;

	return ESP_OK == update_value(IDX_LOCAL_TIME_INFO, buf, sizeof(buf));
}

esp_err_t Gatt_time_svc::update_reference_time_info(void)
{
	const std::tm last_update{SNTP::Sntp::get_instance().time_since_last_update()};
	const std::time_t result = std::time(nullptr);
	const std::tm* local = std::localtime(&result);

	uint8_t time_source{SNTP::Sntp::get_instance().source};
	uint8_t time_accuracy{};
	uint8_t days_since_update{static_cast<uint8_t>(last_update.tm_yday)};
	uint8_t hours_since_update{days_since_update > 0 ? static_cast<uint8_t>(255) : static_cast<uint8_t>(last_update.tm_hour)}; // As per spec; 255 if more than a day

	uint8_t buf[4]{};
	buf[0] = time_source;
	buf[1] = time_accuracy;
	buf[2] = days_since_update;
	buf[3] = hours_since_update;

	return ESP_OK == update_value(IDX_LOCAL_TIME_INFO, buf, sizeof(buf));
}

const esp_gatt_perm_t Gatt_time_svc::permissions[] =
{
	ESP_GATT_PERM_READ,
	ESP_GATT_PERM_READ,
	ESP_GATT_PERM_READ
};

const uint16_t Gatt_time_svc::max_length[GATT_TIME_N_ENTRIES + GATT_TIME_N_NOTIFIES] =
{
	DATA_MAX_LEN,
	DATA_MAX_LEN,
	DATA_MAX_LEN
};

const uint16_t Gatt_time_svc::length[GATT_TIME_N_ENTRIES + GATT_TIME_N_NOTIFIES] =
{
	DATA_MAX_LEN,
	DATA_MAX_LEN,
	DATA_MAX_LEN
};

} // namespace Gatt
