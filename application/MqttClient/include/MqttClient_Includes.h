#include <cstdio>
#include <cstdint>
#include <cstddef>
#include <cstring>
#include <ctime>
#include <algorithm>
#include <string>
#include <utility>
#include <vector>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/semphr.h"
#include "freertos/queue.h"
#include "freertos/event_groups.h"

#pragma once

#include "esp_wifi.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "esp_event.h"
#include "esp_netif.h"

//#include "protocol_examples_common.h"

#include "lwip/sockets.h"
#include "lwip/dns.h"
#include "lwip/netdb.h"
#include "mqtt_client.h"

#include "TaskCPP.h"

#include "Wifi.h"
#include "SntpTime.h"