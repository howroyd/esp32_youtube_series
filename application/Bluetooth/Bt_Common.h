#include <cstring> // memset

#pragma once

#include "include/Bt_Common_Includes.h"

namespace Bt
{

class BtCommon
{
public:
    BtCommon(void)
    {
        ;
    }

    // Interface
    static esp_err_t init_common(esp_bt_mode_t mode=ESP_BT_MODE_IDLE);
    static esp_err_t deinit_common(void);

    // Helper functions
    static char* uuid2str(esp_bt_uuid_t *uuid, char *str, size_t size);
    static char* bda2str(const esp_bd_addr_t bda, char *str, const size_t size);

private:
    enum class init_e { WAIT, GO, DONE };

    static init_e init_called;
    static bool init_success;
    static esp_bt_mode_t mode;
};

} // namespace Bt
// --------------------------------------------------------------------------------------------------------------------
// --------------------------------------------------------------------------------------------------------------------
