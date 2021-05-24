#include "main.h"

#include <thread>

#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#include "esp_log.h"
#define LOG_TAG "MAIN"

static Main my_main;

static void foo(const bool countdown = false)
{
    constexpr static const int max_val{5};
    constexpr static const int min_val{0};
    int ctr{countdown ? max_val : min_val};

    while (true)
    {
        ESP_LOGI(__func__, "%s %d",
                            countdown ? "down" : "up",
                            countdown ? ctr-- : ctr++);
        
        if      (ctr >= max_val) ctr = min_val;
        else if (ctr <= min_val) ctr = max_val;

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(LOG_TAG, "Creating default event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(LOG_TAG, "Initialising NVS");
    ESP_ERROR_CHECK(nvs_flash_init());

    std::thread count_up(foo, false);
    std::thread count_down(foo, true);

    count_up.join();
    count_down.join();
    ESP_LOGI(LOG_TAG, "threads joined main");



    ESP_ERROR_CHECK(my_main.setup());

    while (true)
    {
        my_main.loop();
    }
}

esp_err_t Main::setup(void)
{
    esp_err_t status{ESP_OK};

    ESP_LOGI(LOG_TAG, "Setup!");

    status |= led.init();
    status |= wifi.init();

    if (ESP_OK == status) status |= wifi.begin();

    status |= sntp.init();

    return status;
}

void Main::loop(void)
{
    ESP_LOGI(LOG_TAG, "Hello World!");

    ESP_LOGI(LOG_TAG, "LED on");
    led.set(true);
    vTaskDelay(pdSECOND);

    ESP_LOGI(LOG_TAG, "LED off");
    led.set(false);
    vTaskDelay(pdSECOND);


}