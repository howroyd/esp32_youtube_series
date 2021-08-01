#include "main.h"

static Main my_main;

static void button1_cb(void* arg)
{
    Gpio::GpioOutput* p_led = static_cast<Gpio::GpioOutput*>(arg);
    p_led->set(!p_led->state());
}

static void button2_cb(void* arg)
{
    Gpio::GpioOutput* p_led = static_cast<Gpio::GpioOutput*>(arg);
    p_led->set(!p_led->state());
}

extern "C" void app_main(void)
{
    LOG.infov("Creating default event loop");
    ESP_ERROR_CHECK(esp_event_loop_create_default());

    LOG.infov("Initialising NVS");
    ESP_ERROR_CHECK(nvs_flash_init());

    ESP_ERROR_CHECK(my_main.setup());

    while (true) my_main.loop();
}

esp_err_t Main::setup(void)
{
    esp_err_t status{ESP_OK};

    LOG.infov("Setup!");

    //status |= led.init();
    //status |= wifi.init();

    //if (ESP_OK == status) status |= wifi.begin();

    //status |= sntp.init();

    for (auto& this_led : led) status |= this_led.init();
    //for (auto& this_button : button) status |= this_button.init();

    status |= button[0].init(button1_cb, &led[0]);
    status |= button[1].init(button2_cb, &led[1]);

    status |= pot.init();
    status |= ldr.init();

    if (ESP_OK == status && nullptr == h_task)
    {
        status |= (pdPASS == xTaskCreate(task_blinky, "MultiLedBlink", 2048, this, 5, &h_task)) ? ESP_OK : ESP_ERR_NO_MEM;  
    }

    return status;
}

void Main::loop(void)
{
    static int ctr{0};
    LOG.infov("counter=", ctr++);

    LOG.infov("ADC", pot.pin(), pot.get_filtered(100));
    LOG.infov("LDR", ldr.pin(), ldr.get_filtered(100));

    for (const auto& this_button : button)
        LOG.infov("Button", (int)this_button.pin(), this_button.state() ? "ON" : "OFF");

    vTaskDelay(pdSECOND / 2);
}

TaskHandle_t Main::h_task{nullptr};

void Main::task_blinky(void *pvParameters)
{
    Main* const p_main = static_cast<Main*>(pvParameters);
    esp_err_t status = ESP_OK;

    enum : int { RED, GREEN, BLUE } colour{RED};

    auto enum_to_name = [](int colour)
    {
        switch (colour)
        {
        case RED:   return "RED";
        case GREEN: return "GREEN";
        case BLUE:  return "BLUE";
        default:    return "ERROR";
        };
    };

    auto increment_enum = [&](void)
    {
        switch (colour)
        {
        case RED:   colour = GREEN; break;
        case GREEN: colour = BLUE;  break;
        case BLUE:  colour = RED;   break;
        default:    colour = RED;   break;
        };
    };

    do
    {
        for (auto& this_led : p_main->multicolour_led) status |= this_led.init();
    } while (ESP_OK != status);
    
    while (true)
    {
        for (auto& this_led : p_main->multicolour_led)
        {
            //LOG.infov("LED", enum_to_name(colour), "ON");
            this_led.set(true); vTaskDelay(pdSECOND/4);
            //LOG.infov("LED", enum_to_name(colour), "OFF");
            this_led.set(false);
        }
        vTaskDelay(pdSECOND/4);
    }
}