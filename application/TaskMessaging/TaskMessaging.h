#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_event.h"

#include <cassert>
#include <memory>
#include <mutex>
#include <utility>

namespace TASKMESSAGING
{

/// @brief Declaration of an Interface to a Queue
///
/// Multi-thread and multi-core safe
///
/// @attention Protected constructor; inheritable only
class QueueInterface_t
{
public:
    QueueInterface_t(void) = delete;        ///< Deleted default constructor

    const size_t        queue_len{0};       ///< Number of items the queue can hold
    const size_t        item_n_bytes{0};    ///< Number of bytes per item in the queue
    const size_t        id{0};              ///< Our unique ID of this queue interface instance

    constexpr static const TickType_t wait_ticks{pdMS_TO_TICKS(0)}; ///< Default wait time for FreeRTOS API (send/receive) calls

    /// @brief Bool operator for the class
    ///
    /// e.g. if (instance) { // ... do something }
    ///
    /// @return true if the FreeRTOS queue handle is not nullptr else false
    operator bool() const { return h_queue ? true : false; }

    /// @brief Send an item to the queue
    ///
    /// Item is copied to the queue
    ///
    /// @param[in] item         : item to send to the queue (by copy)
    /// @param[in] wait_ticks   : (optional) max ticks to wait for space to become available
    ///
    /// @return 
    /// 	- ESP_OK if item sent to the queue
    ///     - ESP_ERR_TIMEOUT if a failure in FreeRTOS or timeout
    ///     - ESP_ERR_NO_MEM if the queue is currently full
    ///     - ESP_ERR_INVALID_STATE if the queue is corrupt or not constructed
    template <typename T>
    esp_err_t           send(const T& item, const TickType_t wait_ticks=wait_ticks)
    {
        std::scoped_lock _guard(send_mutx);

        if (*this)
        {
            if (!full() || wait_ticks > 0)
            {
                if (pdPASS == xQueueSend(h_queue.get(), &item, wait_ticks))
                {
                    return ESP_OK;
                }
                return ESP_ERR_TIMEOUT;
            }
            return ESP_ERR_NO_MEM;
        }
        return ESP_ERR_INVALID_STATE;
    }

    /// @brief Send an item to the front of the queue
    ///
    /// Item is copied to the queue
    ///
    /// @param[in] item         : item to send to the queue (by copy)
    /// @param[in] wait_ticks   : (optional) max ticks to wait for space to become available
    ///
    /// @return 
    /// 	- ESP_OK if item sent to the front of the queue
    ///     - ESP_ERR_TIMEOUT if a failure in FreeRTOS or timeout
    ///     - ESP_ERR_NO_MEM if the queue is currently full
    ///     - ESP_ERR_INVALID_STATE if the queue is corrupt or not constructed
    template <typename T>
    esp_err_t           send_to_front(T& item, const TickType_t wait_ticks=wait_ticks)
    {
        std::scoped_lock _guard(send_mutx);

        if (*this)
        {
            if (!full() || wait_ticks > 0)
            {
                if (pdPASS == xQueueSendToFront(h_queue.get(), &item, wait_ticks))
                {
                    return ESP_OK;
                }
                return ESP_ERR_TIMEOUT;
            }
            return ESP_ERR_NO_MEM;
        }
        return ESP_ERR_INVALID_STATE;
    }

    /// @brief Receive the first awaiting item from the queue
    ///
    /// @note Item will be removed from the queue
    ///
    /// @param[out] item        : item received from the queue
    /// @param[in] wait_ticks   : (optional) max ticks to wait for an item
    ///
    /// @return 
    /// 	- ESP_OK if item received from the queue
    ///     - ESP_ERR_TIMEOUT if a timeout waiting an item to arrive
    ///     - ESP_ERR_NOT_FOUND if the queue is empty
    ///     - ESP_ERR_INVALID_STATE if the queue is corrupt or not constructed
    template <typename T>
    esp_err_t           receive(T& item, const TickType_t wait_ticks=wait_ticks)
    {
        std::scoped_lock _guard(receive_mutx);

        if (*this)
        {
            if (!empty() || wait_ticks > 0)
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

    /// @brief Receive the first awaiting item from the queue without removing it
    ///
    /// @note Item will not be removed from the queue
    ///
    /// @param[out] item        : item received from the queue
    /// @param[in] wait_ticks   : (optional) max ticks to wait for an item
    ///
    /// @return 
    /// 	- ESP_OK if item received from the queue
    ///     - ESP_ERR_TIMEOUT if a timeout waiting an item to arrive
    ///     - ESP_ERR_NOT_FOUND if the queue is empty
    ///     - ESP_ERR_INVALID_STATE if the queue is corrupt or not constructed
    template <typename T>
    esp_err_t           peek(T& item, const TickType_t wait_ticks=wait_ticks) const
    {
        std::scoped_lock _guard(receive_mutx);

        if (*this)
        {
            if (!empty() || wait_ticks > 0)
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

    /// @brief Number of items currently in the queue
    ///
    /// @return Number of items currently in the queue
    size_t              n_items_waiting(void)   const
    {
        std::scoped_lock _guard(receive_mutx);

        if (h_queue)
            return uxQueueMessagesWaiting(h_queue.get());
        return 0;
    }

    /// @brief Number of free spaces in the queue
    ///
    /// @return Number of free spaces in the queue
    size_t              n_free_spaces(void)     const
    {
        std::scoped_lock _guard(receive_mutx);

        if (h_queue)
            return uxQueueSpacesAvailable(h_queue.get());
        return 0;
    }

    /// @brief Is the queue currently empty
    ///
    /// @return true if the queue is empty else false
    bool                empty(void)             const
        { return 0 == n_items_waiting(); }

    /// @brief Is the queue currently full
    ///
    /// @return true if the queue is full else false
    bool                full(void)              const 
        { return 0 == n_free_spaces(); }

    /// @brief Remove all items (if any) currently in the queue
    ///
    /// @return true if success else false
    bool                clear(void)
    { 
        std::scoped_lock _guard(send_mutx, receive_mutx);

        return pdPASS == xQueueReset(h_queue.get()); 
    }

protected:
    /// @brief Construct a queue interface
    ///
    /// @attention Protected constructor; inheritable only
    ///
    /// @param[in] h_queue      : handle to a FreeRTOS queue
    /// @param[in] n_items      : number of items the queue can hold
    /// @param[in] item_n_bytes : number of bytes that one item requires
    /// @param[in] id           : our unique ID of this queue
    QueueInterface_t(std::shared_ptr<QueueDefinition> h_queue,
                        const size_t n_items,
                        const size_t item_n_bytes, 
                        const size_t id) :
        queue_len{n_items},
        item_n_bytes{item_n_bytes},
        id{id},
        h_queue{h_queue}        
    {}

    /// @brief Copy an existing instance of an interface
    ///
    /// @attention Protected copy constructor
    ///
    /// @param[in] other : an instance of this class to copy
    QueueInterface_t(const QueueInterface_t& other) :
        queue_len{other.queue_len},
        item_n_bytes{other.item_n_bytes},
        id{other.id},
        h_queue{other.h_queue}
    {
        assert(0 < queue_len); assert(0 < item_n_bytes);
    }

    std::shared_ptr<QueueDefinition>    h_queue;        ///< FreeRTOS handle to the queue
    mutable std::recursive_mutex        send_mutx{};    ///< Mutex to protect sending to the queue
    mutable std::recursive_mutex        receive_mutx{}; ///< Mutex to protect receiving from the queue
};

/// @brief Declaration of a Dynamically Allocated Queue
///
/// Multi-thread and multi-core safe
///
/// @note Inherits the queue interface
class DynamicQueue_t : public QueueInterface_t
{
public:
    /// @brief Construct a dynamic queue
    ///
    /// @param[in] n_items      : number of items the queue can hold
    /// @param[in] item_n_bytes : number of bytes that one item requires
    /// @param[in] id           : our unique ID of this queue
    DynamicQueue_t(const size_t n_items,
                    const size_t item_n_bytes, 
                    const size_t id) :
        QueueInterface_t{std::move(create_queue_shared_ptr(n_items, item_n_bytes)),
                            n_items, item_n_bytes, id}
    {}

    DynamicQueue_t(void) = delete; ///< Deleted default constructor

private:
    /// @brief Construct a FreeRTOS queue on the heap
    ///
    /// @param[in] n_items      : number of items the queue should hold
    /// @param[in] item_n_bytes : number of bytes that one item requires
    ///
    /// @return FreeRTOS queue handle
    QueueHandle_t create_queue(const size_t n_items, const size_t item_n_bytes)
    {
        return xQueueCreate(n_items, item_n_bytes);
    }

    /// @brief Delete a FreeRTOS queue from the heap
    ///
    /// @param[in] h : FreeRTOS queue handle
    void delete_queue(QueueHandle_t h)
    {
        std::scoped_lock _guard(send_mutx, receive_mutx);
        vQueueDelete(h);
    }

    /// @brief Construct a FreeRTOS queue on the heap and return a std::shared_ptr to it
    ///
    /// Custom destructor ensures when the last instance of the std::shared_ptr goes out
    ///  of scope, the FreeRTOS queue will be deleted
    ///
    /// @param[in] n_items      : number of items the queue should hold
    /// @param[in] item_n_bytes : number of bytes that one item requires
    ///
    /// @return A managed std::shared_ptr to the FreeRTOS queue
    std::shared_ptr<QueueDefinition> create_queue_shared_ptr(const size_t n_items, const size_t item_n_bytes)
    {
        return {create_queue(n_items, item_n_bytes), 
                    [this](auto h){delete_queue(h);}};
    }
};

/// @brief Declaration of a Stack or Statically Allocated Queue
///
/// Multi-thread and multi-core safe
///
/// @note Inherits the queue interface
///
/// @param[in] T    : typename of the items the queue will hold
/// @param[in] len  : number of items the queue can hold
template<typename T, size_t len>
class StaticQueue_t  : public QueueInterface_t
{
public:
    /// @brief Construct a stack or statically allocated queue
    ///
    /// @param[in] id : our unique ID of this queue
    StaticQueue_t(const size_t id) :
        QueueInterface_t{create_queue(len, sizeof(T)),
                            len, sizeof(T), id}
    {}

    StaticQueue_t(void) = delete; ///< Deleted default constructor

    // TODO what if this class was destroyed but it had been copied?
    StaticQueue_t(const StaticQueue_t&) = delete; ///< Deleted copy constructor

private:
    /// @brief Construct a FreeRTOS queue on the stack or statically (compile time)
    ///
    /// @param[in] n_items      : number of items the queue should hold
    /// @param[in] item_n_bytes : number of bytes that one item requires
    ///
    /// @return FreeRTOS static queue handle
    QueueHandle_t create_queue(const size_t n_items, const size_t item_n_bytes)
    {
        return xQueueCreateStatic(n_items, item_n_bytes,
                                    queue_storage, &queue_control_block);
    }

    /// @brief Delete a FreeRTOS queue from the stack
    ///
    /// @note Safe to call if the queue is statically allocated, it will just be
    ///        removed from the FreeRTOS registry but the memory obviously won't
    ///        be deleted
    ///
    /// @param[in] h : FreeRTOS static queue handle
    void delete_queue(QueueHandle_t h)
    {
        std::scoped_lock _guard(send_mutx, receive_mutx);
        vQueueDelete(h);
    }

    /// @brief Construct a FreeRTOS queue on the stack opr statically and return a std::shared_ptr to it
    ///
    /// Custom destructor ensures when the last instance of the std::shared_ptr goes out
    ///  of scope, the FreeRTOS queue will be deleted
    ///
    /// @param[in] n_items      : number of items the queue should hold
    /// @param[in] item_n_bytes : number of bytes that one item requires
    ///
    /// @return A managed std::shared_ptr to the FreeRTOS queue
    std::shared_ptr<QueueDefinition> create_queue_shared_ptr(const size_t n_items, const size_t item_n_bytes)
    {
        return {create_queue(n_items, item_n_bytes), 
                    [this](auto h){delete_queue(h);}};
    }

    StaticQueue_t   queue_control_block{};              ///< Memory used by FreeRTOS for the queue management
    uint8_t         queue_storage[len * sizeof(T)]{};   ///< Memory used by FreeRTOS to hold the queue items
};

} // namespace TASKMESSAGING