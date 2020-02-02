#include <cstring> // memset
#include <string>

#pragma once

#include "Bt_Classic_Includes.h"

namespace Bt_Classic
{

class BtClassic final : TaskClass, Bt::BtCommon
{
    enum app_gap_state_e
    {
        APP_GAP_STATE_IDLE = 0,
        APP_GAP_STATE_DEVICE_DISCOVERING,
        APP_GAP_STATE_DEVICE_DISCOVER_COMPLETE,
        APP_GAP_STATE_SERVICE_DISCOVERING,
        APP_GAP_STATE_SERVICE_DISCOVER_COMPLETE,
    };

public:
    struct app_gap_cb_t
    {
        bool dev_found;
        uint8_t bdname_len;
        uint8_t eir_len;
        uint8_t rssi;
        uint32_t cod;
        uint8_t eir[ESP_BT_GAP_EIR_DATA_LEN];
        uint8_t bdname[ESP_BT_GAP_MAX_BDNAME_LEN + 1];
        esp_bd_addr_t bda;
        app_gap_state_e state;
    };

    static app_gap_cb_t m_dev_info;

    BtClassic(esp_bt_mode_t mode = ESP_BT_MODE_CLASSIC_BT) : 
            TaskClass("BT_classic_task", TaskPrio_Lowest, 3066)
    {
        this->mode = mode;
    }

protected:
    // Methods & functions
	esp_err_t init(void);
	esp_err_t deinit(void);
	static esp_err_t advertise(const bool status=true);

private:
    void task(void);

    // Generic
    static bool get_name_from_eir(uint8_t *eir, uint8_t *bdname, uint8_t *bdname_len);

    static void update_device_info(esp_bt_gap_cb_param_t *param);

    static void bt_app_gap_init(void);
    static void bt_app_gap_cb(esp_bt_gap_cb_event_t event, esp_bt_gap_cb_param_t *param);
    static void bt_app_gap_start_up(void);

    // SPP
    static void esp_spp_cb(esp_spp_cb_event_t event, esp_spp_cb_param_t *param);

    static const esp_spp_mode_t esp_spp_mode;

    static long data_num;

    static const esp_spp_sec_t sec_mask;
    static const esp_spp_role_t role_slave;

    static esp_bt_mode_t mode;
};

} // namespace Bt_Classic
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
