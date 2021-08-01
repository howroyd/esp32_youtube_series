#include "main.h"

#include <thread>

#define LOG_LEVEL_LOCAL ESP_LOG_VERBOSE
#include "esp_log.h"
#define LOG_TAG "MAIN"

static Main my_main;

static void foo(const bool countdown = false)
{
    const char* instance_cstr = countdown ? "down" : "up";
    constexpr static const int max_val{5};
    constexpr static const int min_val{0};
    int ctr{countdown ? max_val : min_val};

    char buf[100]{};

    while (true)
    {
        {
            std::unique_lock<std::recursive_timed_mutex> loglock;
            if (ESP_OK == LOG.lock(loglock, std::chrono::milliseconds(250)))
            {
                sprintf(buf, "%s %d",
                                instance_cstr,
                                countdown ? ctr-- : ctr++);

                LOG.info(buf);

                if      (ctr >= max_val) ctr = min_val;
                else if (ctr <= min_val) ctr = max_val;

                vTaskDelay(pdMS_TO_TICKS(750));

                LOG.info(std::string("Releasing the logging lock for ") + instance_cstr);
            }
            else LOG.warning(std::string("Failed to aquire logging lock for ") + instance_cstr);
        }

        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(LOG_TAG, "Creating default event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    ESP_LOGI(LOG_TAG, "Initialising NVS");
    ESP_ERROR_CHECK(nvs_flash_init());

    //LOG.logf(ESP_LOG_INFO, "%d %s %s", 42, "hello", "world");
    LOG.infov(42, "hello", "world");

    //std::thread count_up(foo, false);
    //std::thread count_down(foo, true);

    //count_up.join();
    //count_down.join();
    //ESP_LOGI(LOG_TAG, "threads joined main");

    ESP_ERROR_CHECK(my_main.setup());

    int ctr = 0;

    while (true)
    {
        LOG.infov("counter=", ctr++);
        //ESP_LOGI("LOG", "%s", stream.str().c_str());
        my_main.loop();
    }
}

esp_err_t Main::setup(void)
{
    esp_err_t status{ESP_OK};

    ESP_LOGI(LOG_TAG, "Setup!");

    //status |= led.init();
    //status |= wifi.init();

    //if (ESP_OK == status) status |= wifi.begin();

    //status |= sntp.init();

    for (auto& led : multicolour_led)
    {
        status |= led.init();
    }

    status |= pot.init();

    return status;
}

void Main::loop(void)
{
    LOG.infov("ADC", pot.get(100));

    enum : int { RED, GREEN, BLUE } colour{RED};

    auto enum_to_name = [](int colour)
    {
        switch (colour)
        {
        case RED: return "RED";
        case GREEN: return "GREEN";
        case BLUE: return "BLUE";
        default: return "ERROR";
        };
    };

    auto increment_enum = [&](void)
    {
        switch (colour)
        {
        case RED: colour = GREEN; break;
        case GREEN: colour = BLUE; break;
        case BLUE: colour = RED; break;
        default: colour = RED; break;
        };
    };

    for (auto& led : multicolour_led)
    {
        //LOG.infov("LED", enum_to_name(colour), "ON");
        led.set(true); vTaskDelay(pdSECOND/4);
        //LOG.infov("LED", enum_to_name(colour), "OFF");
        led.set(false); vTaskDelay(pdSECOND/4);
        increment_enum();
    }
}