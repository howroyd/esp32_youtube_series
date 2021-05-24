#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"

#include <cassert>
#include <memory>
#include <mutex>
//#include <vector>

namespace TASKMESSAGING
{

class DynamicQueue_t
{
    //friend class TaskMessagingBase; // TODO might not be req.

public:
    const size_t        queue_len{0};
    const size_t        item_n_bytes{0};
    const size_t        id{0};

    constexpr static const TickType_t wait_ticks{pdMS_TO_TICKS(1000)};

    operator bool() const { return h_queue ? true : false; }

    template <typename T>
    esp_err_t           send(T& item, const TickType_t wait_ticks=wait_ticks)
    {
        std::lock_guard<std::recursive_mutex> _guard(mutx);

        if (*this)
        {
            if (!full())
            {
                if (pdPASS == xQueueSend(h_queue.get(), &item, wait_ticks))
                {
                    return ESP_OK;
                }
                return ESP_FAIL;
            }
            return ESP_ERR_NO_MEM;
        }
        return ESP_ERR_INVALID_STATE;
    }

    template <typename T>
    esp_err_t           send_to_front(T& item, const TickType_t wait_ticks=wait_ticks)
    {
        std::lock_guard<std::recursive_mutex> _guard(mutx);

        if (*this)
        {
            if (!full())
            {
                if (pdPASS == xQueueSendToFront(h_queue.get(), &item, wait_ticks))
                {
                    return ESP_OK;
                }
                return ESP_FAIL;
            }
            return ESP_ERR_NO_MEM;
        }
        return ESP_ERR_INVALID_STATE;
    }

    template <typename T>
    esp_err_t           receive(T& item, const TickType_t wait_ticks=wait_ticks)
    {
        std::lock_guard<std::recursive_mutex> _guard(mutx);

        if (*this)
        {
            if (!empty())
            {
                if (pdPASS == xQueueReceive(h_queue.get(), &item, wait_ticks))
                {
                    return ESP_OK;
                }
                return ESP_ERR_TIMEOUT;
            }
            return ESP_ERR_NOT_FOUND;
        }
        return ESP_ERR_INVALID_STATE;
    }

    template <typename T>
    esp_err_t           peek(T& item, const TickType_t wait_ticks=wait_ticks) const
    {
        std::lock_guard<std::recursive_mutex> _guard(mutx);

        if (*this)
        {
            if (!empty())
            {
                if (pdPASS == xQueuePeek(h_queue.get(), &item, wait_ticks))
                {
                    return ESP_OK;
                }
                return ESP_ERR_TIMEOUT;
            }
            return ESP_ERR_NOT_FOUND;
        }
        return ESP_ERR_INVALID_STATE;
    }

    size_t              n_items_waiting(void)   const
    {
        std::lock_guard<std::recursive_mutex> _guard(mutx);

        if (h_queue)
            return uxQueueMessagesWaiting(h_queue.get());
        return 0;
    }

    size_t              n_free_spaces(void)     const
    {
        std::lock_guard<std::recursive_mutex> _guard(mutx);

        if (h_queue)
            return uxQueueSpacesAvailable(h_queue.get());
        return 0;
    }

    bool                empty(void)             const
        { return 0 == n_items_waiting(); }

    bool                full(void)              const 
        { return 0 == n_free_spaces(); }

    bool                clear(void)
    { 
        std::lock_guard<std::recursive_mutex> _guard(mutx);

        return pdPASS == xQueueReset(h_queue.get()); 
    }

    DynamicQueue_t(void) = delete;

    DynamicQueue_t(const size_t n_items,
                    const size_t item_n_bytes, 
                    const size_t id) :
        queue_len{n_items},
        item_n_bytes{item_n_bytes},
        id{id},
        h_queue(xQueueCreate(this->queue_len, this->item_n_bytes), 
                [](auto h){vQueueDelete(h);})
    {
        assert(0 < queue_len); assert(0 < item_n_bytes);
    }

    DynamicQueue_t(const DynamicQueue_t& other) :
        queue_len{other.queue_len},
        item_n_bytes{other.item_n_bytes},
        id{other.id},
        h_queue{other.h_queue}
    {}

protected:
    std::shared_ptr<QueueDefinition>    h_queue;
    mutable std::recursive_mutex        mutx{};
};


} // namespace TASKMESSAGING