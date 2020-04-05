#include <cstring> // memcpy

#pragma once

#include "include/Ble_Includes.h"

#define BLE_GATTS_MANUF_NAME "GreenGiant"
#define BLE_GATTS_MODEL "Develop"
#define BLE_GATTS_SERIAL_STR "XXXXXX"
#define BLE_GATTS_HW "1"
#define BLE_GATTS_FW "1"

namespace Bt_Le
{

// --------------------------------------------------------------------------------------------------------------------
class Ble final: public TaskClass, Bt::BtCommon
{
	// ID of public full advertising UUID
	static constexpr uint8_t gap_id_uuid = 0x07;

	// ID of public full advertising name string
	static constexpr uint8_t gap_id_name = 0x08;

	static constexpr uint8_t uuid[] = 
	{
		0xC8, 0x37, 0x80, 0x1F, 0x83, 0x29, 0x46, 0x58, 0xB6, 0x11, 0x9F, 0x53, 0x7F, 0x73, 0xE8, 0x20
	};

	Ble(esp_bt_mode_t mode = ESP_BT_MODE_BLE) :
	    	TaskClass("Ble_task", TaskPrio_Mid, 1024 * 6),
			state{state_machine_t::OFF}
	{
		this->mode = mode;
	}

	~Ble(void)
	{
		deinit();
	}

public:
	bool start = false;
	bool running = false;

	static Ble& get_instance(esp_bt_mode_t mode = ESP_BT_MODE_BLE)
	{
		static Ble instance(mode);
		return instance;
	}

	static bool change_advertising_uuid(const esp_bt_uuid_t uuid);
	static bool change_advertising_name(const char* new_name);
	static bool change_serial_number(const uint32_t serial_24bit);
	static bool change_serial_number(const uint8_t serial_24bit[6]);

	// Send a C style string to a connected device
    static esp_err_t send_cstring(const char* str);

	// GATT service list
	enum : uint16_t
	{
		IDX_DEV_INFO,
		IDX_SPP,
		IDX_HUB_INFO,
		IDX_TIME,

		IDX_N_IDX_ENTRIES
	};

	// Notifications 
    enum class bt_notifs_t : uint32_t
	{
		INITIALISED = 0x80,
		CONNECTED = 0x11,
		DISCONNECTED = 0x21,
	};

    enum class bt_spp_notifs_t : uint32_t
	{
		NEW_STRING = 0x41
	};

protected:
	// Methods & functions
	esp_err_t init(void);
	esp_err_t deinit(void);
	static esp_err_t advertise(const bool status=true);

	// High level settings
	static constexpr const uint8_t ESP_APP_ID = 0x56;
    static esp_bt_mode_t mode;

	// Advertising Data
    static esp_ble_adv_params_t adv_params;
	static constexpr uint8_t adv_data_min[] = {0x02, 0x01, 0x06};
	static uint8_t adv_data_raw[];
    static bool adv_state; // Advertising state

	// GATT Services
	static Gatt::Gatt_dev_info_svc dev_info;
	static Gatt::Gatt_spp_svc spp;
	static Gatt::Gatt_hub_info_svc hub_info;
	static Gatt::Gatt_time_svc time_info;

private:
	// Task Class and Mediator specific
	void task(void);
	//void handle_msg(hub::MediatorMessage* msg);

	// Notifications and states
    enum class state_machine_t : uint32_t
	{
		OFF,
		SETUP,
		READY,
		CONNECTED,
		DISCONNECTED, // Use for internal notifications only
		NEW_DATA_FROM_BT, // Use for internal notifications only
		NEW_DATA_FROM_MEDIATOR,
		TESTING_RADIO_OFF,
		TESTING_RADIO_ON,
		UNKNOWN = ULONG_MAX
	} state;

	// Event handlers & callbacks
	static void gap_event_handler(esp_gap_ble_cb_event_t event, esp_ble_gap_cb_param_t *param);
	static void gatts_profile_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);
	static void gatts_event_handler(esp_gatts_cb_event_t event, esp_gatt_if_t gatts_if, esp_ble_gatts_cb_param_t *param);

	// ----------------------------------------------------------------------------------------------------------------
    // GATTS specific
    // ----------------------------------------------------------------------------------------------------------------
    struct gatts_profile_inst
    {
        esp_gatts_cb_t gatts_cb;
        uint16_t gatts_if;
		/*
        uint16_t app_id;
        uint16_t conn_id;
        uint16_t service_handle;
        esp_gatt_srvc_id_t service_id;
        uint16_t char_handle;
        esp_bt_uuid_t char_uuid;
        esp_gatt_perm_t perm;
        esp_gatt_char_prop_t property;
        uint16_t descr_handle;
        esp_bt_uuid_t descr_uuid;
		*/
    };

	// One gatt-based profile one app_id and one gatts_if, this array will store the gatts_if returned by ESP_GATTS_REG_EVT
    static struct gatts_profile_inst profile_tab[IDX_N_IDX_ENTRIES];

	// SPP specific
    static bool enable_data_ntf;
};
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------

} // namespace Bt_Le
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------