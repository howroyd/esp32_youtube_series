#include "main.h"

static Main my_main;
static std::array<Gpio::GpioOutput, 2>* p_led = {nullptr};

static void button1_cb(void*)
{
    (*p_led)[0].set(!(*p_led)[0].state());
}

static void button2_cb(void*)
{
    (*p_led)[1].set(!(*p_led)[1].state());
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
    p_led = &led;

    //status |= led.init();
    //status |= wifi.init();

    //if (ESP_OK == status) status |= wifi.begin();

    //status |= sntp.init();

    for (auto& this_led : led) status |= this_led.init();
    for (auto& this_led : multicolour_led) status |= this_led.init();
    //for (auto& this_button : button) status |= this_button.init();

    status |= button[0].init(button1_cb);
    status |= button[1].init(button2_cb);

    status |= pot.init();
    status |= ldr.init();

    return status;
}

void Main::loop(void)
{
    static int ctr{0};
    LOG.infov("counter=", ctr++);

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
        case GREEN: colour = BLUE; break;
        case BLUE:  colour = RED; break;
        default:    colour = RED; break;
        };
    };

    LOG.infov("ADC", pot.get());
    LOG.infov("LDR", ldr.get());

    for (auto& this_led : multicolour_led)
    {
        //LOG.infov("LED", enum_to_name(colour), "ON");
        this_led.set(true); vTaskDelay(pdSECOND/4);
        //LOG.infov("LED", enum_to_name(colour), "OFF");
        this_led.set(false); vTaskDelay(pdSECOND/4);
    }

    for (const auto& this_button : button)
        LOG.infov("Button", (int)this_button.pin(), this_button.state() ? "ON" : "OFF");
}