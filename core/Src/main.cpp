#include "main.h"
#include <chrono>

#include <functional>
#include <list>
#include <queue>
#include <vector>
#include <array>
#include <iostream>
class QueueTest
{
public:
    enum class Priority
    {
        LOW,
        MEDIUM,
        HIGH
    };

    constexpr static std::array<Priority, 3> list_of_p{
        Priority::HIGH,
        Priority::MEDIUM,
        Priority::LOW};

    constexpr static const char *prio_to_str(const Priority p)
    {
        switch (p)
        {
        case Priority::HIGH:
            return "High";
        case Priority::MEDIUM:
            return "Medium";
        case Priority::LOW:
            return "Low";
        }
        return "";
    }

    enum class Led
    {
        GREEN,
        YELLOW,
        RED
    };

    constexpr static std::array<Led, 3> list_of_l{
        Led::RED,
        Led::YELLOW,
        Led::GREEN};

    constexpr static const char *led_to_str(const Led l)
    {
        switch (l)
        {
        case Led::RED:
            return "Red";
        case Led::YELLOW:
            return "Yellow";
        case Led::GREEN:
            return "Green";
        }
        return "";
    }

    enum class Buzzer
    {
        OFF,
        PULSE,
        CONSTANT
    };

    constexpr static std::array<Buzzer, 3> list_of_b{
        Buzzer::OFF,
        Buzzer::PULSE,
        Buzzer::CONSTANT};

    constexpr static const char *buzz_to_str(const Buzzer b)
    {
        switch (b)
        {
        case Buzzer::OFF:
            return "Off";
        case Buzzer::PULSE:
            return "Pulse";
        case Buzzer::CONSTANT:
            return "Constant";
        }
        return "";
    }

    struct Alert
    {
        Priority priority;
        Led led;
        Buzzer buzzer;

        constexpr Alert(const Priority priority,
                        const Led led,
                        const Buzzer buzzer) noexcept : priority{priority},
                                                        led{led},
                                                        buzzer{buzzer}
        {
        }

        Alert(void) = delete;
        Alert(const Alert &) noexcept = default;
        Alert(Alert &&) noexcept = default;
        Alert &operator=(const Alert &) noexcept = default;
        Alert &operator=(Alert &&) noexcept = default;

        constexpr bool operator<(const Alert &other) const
        {
            if (other.priority == priority)
            {
                if (other.led == led)
                {
                    return buzzer < other.buzzer;
                }
                return led < other.led;
            }
            return priority < other.priority;
        }
        constexpr bool operator>(const Alert &other) const { return *this > other; }
        constexpr bool operator<=(const Alert &other) const { return !(*this > other); }
        constexpr bool operator>=(const Alert &other) const { return !(*this < other); }

        constexpr bool operator==(const Alert &other) const
        {
            return priority == other.priority &&
                   led == other.led &&
                   buzzer == other.buzzer;
        }
        constexpr bool operator!=(const Alert &other) const { return !(*this == other); }

        constexpr static bool predicate(const Alert &left, const Alert &right)
        {
            return left > right;
            if (left.priority == right.priority)
            {
                if (left.led == right.led)
                {
                    return left.buzzer < right.buzzer;
                }
                return left.led > right.led;
            }
            return left.priority > right.priority;
        }
    };
};
using queue_item_t = QueueTest::Alert ;
using container_t = std::vector<queue_item_t>;
using queue_t = std::priority_queue<queue_item_t, container_t, std::less<queue_item_t>>;
queue_t *queue;

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

    container_t mem{};
    mem.reserve(24);
    queue = new queue_t{std::less<queue_item_t>(), std::move(mem)};

    for (auto b : QueueTest::list_of_b)
    {
        for (auto l : QueueTest::list_of_l)
        {
            for (auto p : QueueTest::list_of_p)
                queue->push({p, l, b});
        }
    }

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
    using std::chrono::high_resolution_clock;
    using std::chrono::duration_cast;
    using std::chrono::duration;
    using std::chrono::milliseconds;

    static int ctr{0};
    LOG.infov("counter=", ctr++);

    const auto t1 = high_resolution_clock::now();
        const uint32_t pot_val = pot.get_filtered(200);
    const auto t2 = high_resolution_clock::now();
    
    const auto ms_int = duration_cast<milliseconds>(t2 - t1);
    const duration<double, std::milli> ms_double = t2 - t1;

    LOG.infov("ADC", pot.pin(), pot_val, "\t(", ms_double.count(), ")");
    LOG.infov("LDR", ldr.pin(), ldr.get_filtered(200));

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



