#pragma once

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"

#include <cassert>
#include <memory>
#include <mutex>
#include <utility>

namespace TASKMESSAGING
{

/// @brief Generic Interface to a FreeRTOS Queue
class QueueInterface
{
public:
    const size_t queue_len{0};          ///< Number of items the queue can hold
    const size_t item_size_bytes{0};    ///< Number of bytes per item

	/// @brief Bool operator
	///
	/// @return 
	/// 	- true if underlying queue handle is valid, else false
    operator bool() const { return h_queue ? true : false; }

    QueueInterface(void) = delete;      ///< Not default constructable

	/// @brief Send an item to the queue
	///
    /// @param[in] item : item to post to the queue (by copy)
    ///
	/// @return 
	/// 	- ESP_OK if item sent to queue
    ///     - ESP_ERR_INVALID_STATE is queue is invalid
    ///     - ESP_ERR_NO_MEM if queue is full
	/// 	- ESP_FAIL if other FreeRTOS queue send error
    template <typename T>
    esp_err_t send(const T& item)
    {
        std::scoped_lock _guard(_send_mutex);

        if (*this)
        {
            if (!full())
            {
                if (pdPASS == xQueueSend(h_queue.get(), &item, 0))
                {
                    return ESP_OK;
                }
                return ESP_FAIL;
            }
            return ESP_ERR_NO_MEM;
        }
        return ESP_ERR_INVALID_STATE;
    }

	/// @brief Send an item to the front of the queue
	///
    /// @param[in] item : item to post to the queue (by copy)
    ///
	/// @return 
	/// 	- ESP_OK if item sent to queue
    ///     - ESP_ERR_INVALID_STATE is queue is invalid
    ///     - ESP_ERR_NO_MEM if queue is full
	/// 	- ESP_FAIL if other FreeRTOS queue send error
    template <typename T>
    esp_err_t send_to_front(const T& item)
    {
        std::scoped_lock _guard(_send_mutex);

        if (*this)
        {
            if (!full())
            {
                if (pdPASS == xQueueSendToFront(h_queue.get(), &item, 0))
                {
                    return ESP_OK;
                }
                return ESP_FAIL;
            }
            return ESP_ERR_NO_MEM;
        }
        return ESP_ERR_INVALID_STATE;
    }

	/// @brief Receive an item from the queue
	///
    /// @param[out] item : item received from the queue
    ///
	/// @return 
	/// 	- ESP_OK if item received from queue
    ///     - ESP_ERR_INVALID_STATE is queue is invalid
    ///     - ESP_ERR_NOT_FOUND if queue is empty
	/// 	- ESP_FAIL if other FreeRTOS queue receive error
    template <typename T>
    esp_err_t receive(T& item)
    {
        std::scoped_lock _guard(_receive_mutex);

        if (*this)
        {
            if (!empty())
            {
                if (pdPASS == xQueueReceive(h_queue.get(), &item, 0))
                {
                    return ESP_OK;
                }
                return ESP_FAIL;
            }
            return ESP_ERR_NOT_FOUND;
        }
        return ESP_ERR_INVALID_STATE;
    }

	/// @brief Receive an item from the queue without removing it
	///
    /// @param[out] item : item received from the queue
    ///
	/// @return 
	/// 	- ESP_OK if item received from queue
    ///     - ESP_ERR_INVALID_STATE is queue is invalid
    ///     - ESP_ERR_NOT_FOUND if queue is empty
	/// 	- ESP_FAIL if other FreeRTOS queue receive error
    template <typename T>
    esp_err_t peek(T& item)
    {
        std::scoped_lock _guard(_receive_mutex);

        if (*this)
        {
            if (!empty())
            {
                if (pdPASS == xQueuePeek(h_queue.get(), &item, 0))
                {
                    return ESP_OK;
                }
                return ESP_FAIL;
            }
            return ESP_ERR_NOT_FOUND;
        }
        return ESP_ERR_INVALID_STATE;
    }

	/// @brief Number of items currently in the queue
	///
	/// @return number of unread items in the queue
    size_t n_items_waiting(void) const
    {
        std::scoped_lock _guard(_send_mutex, _receive_mutex);

        if (*this)
            return uxQueueMessagesWaiting(h_queue.get());
        
        return 0;
    }

	/// @brief Number of free spaces in the queue
	///
	/// @return number of free spaces in the queue
    size_t n_free_spaces(void) const
    {
        std::scoped_lock _guard(_send_mutex, _receive_mutex);

        if (*this)
            return uxQueueSpacesAvailable(h_queue.get());
        
        return 0;
    }

    bool empty(void) const { return 0 == n_items_waiting(); } ///< True if queue is empty
    bool full(void)  const { return 0 == n_free_spaces();   } ///< True if queue is full

	/// @brief Remove all items from the queue
	///
	/// @return true if queue emptied, false if queue is invalid
    bool clear(void)
    {
        std::scoped_lock _guard(_send_mutex, _receive_mutex);

        if (*this)
            return pdPASS == xQueueReset(h_queue.get());
        
        return false;
    }

protected:
	/// @brief Protected Constructor (inheritable only)
	///
    /// @param[in] h_queue      : unique pointer containing the FreeRTOS queue handle
    /// @param[in] n_items      : number of items the queue can hold
    /// @param[in] item_n_bytes : number of bytes per item
    QueueInterface(std::unique_ptr<QueueDefinition> h_queue,
                    const size_t n_items,
                    const size_t item_n_bytes) :
        queue_len{n_items},
        item_size_bytes{item_n_bytes},
        h_queue{h_queue}
    {
        assert(0 < n_items); assert(0 < item_n_bytes); // TODO review this
    }

	/// @brief Protected Copy Constructor
	///
    /// Will create a threadsafe copy referring to the same underlying queue
    ///
    /// @note e.g. one copy for the sender and one for the receiver
    ///
    /// @param[in] other        : object to copy
    QueueInterface(const QueueInterface& other) :
        queue_len{other.n_items},
        item_size_bytes{other.item_n_bytes},
        h_queue{other.h_queue}
    {}

    std::shared_ptr<QueueDefinition>    h_queue{};          ///< Shared pointer containing the FreeRTOS queue
    mutable std::recursive_mutex        _send_mutex{};      ///< Mutex to prevent multiple senders at the same time
    mutable std::recursive_mutex        _receive_mutex{};   ///< Mutex to prevent multiple readers at the same time
};

/// @brief Dynamically Allocated, Threadsafe FreeRTOS Queue
///
/// @note Inherits the generic queue interface
class DynamicQueue : public QueueInterface
{
public:
	/// @brief Constructor
	///
    /// @param[in] n_items      : number of items the queue can hold
    /// @param[in] item_n_bytes : number of bytes per item
    DynamicQueue(const size_t n_items,
                    const size_t item_n_bytes) :
        QueueInterface{std::move(create_queue_unique_ptr(n_items, item_n_bytes)), n_items, item_n_bytes}
    {}

private:
	/// @brief Create the FreeRTOS queue
    ///
    /// @param[in] n_items      : number of items the queue can hold
    /// @param[in] item_n_bytes : number of bytes per item
    QueueHandle_t create_queue(const size_t n_items, const size_t item_n_bytes)
    {
        return xQueueCreate(n_items, item_n_bytes);
    }

	/// @brief Delete the FreeRTOS queue
    ///
    /// @param[in] h            : FreeRTOS queue handle of the queue to delete
    void delete_queue(QueueHandle_t h)
    {
        std::scoped_lock _guard(send_mutx, receive_mutx);
        vQueueDelete(h);
    }

	/// @brief Create the FreeRTOS queue as a unique pointer with deleter
    ///
    /// @note The deleter will be called when the pointer goes out of scope
    ///
    /// @param[in] n_items      : number of items the queue can hold
    /// @param[in] item_n_bytes : number of bytes per item
    std::unique_ptr<QueueDefinition> create_queue_unique_ptr(const size_t n_items, const size_t item_n_bytes)
    {
        return { create_queue(n_items, item_n_bytes),
                    [this](auto h){ delete_queue(h); }};
    }
};

/// @brief Statically or stack based, Threadsafe FreeRTOS Queue
///
/// @note Inherits the generic queue interface
///
/// @param[in] T       : datatype of the items in the queue
/// @param[in] n_items : number of items the queue can hold
template <typename T, size_t n_items>
class StaticQueue : public QueueInterface
{
public:
	/// @brief Constructor
    StaticQueue(void) :
        QueueInterface{std::move(create_queue_unique_ptr(n_items, sizeof(T))), n_items, sizeof(T)}
    {}

private:
	/// @brief Create the FreeRTOS queue
    ///
    /// @param[in] n_items      : number of items the queue can hold
    /// @param[in] item_n_bytes : number of bytes per item
    QueueHandle_t create_queue(const size_t n_items, const size_t item_n_bytes)
    {
        return xQueueCreateStatic(n_items, item_n_bytes, queue_storage, &queue_control_block);
    }

	/// @brief Delete the FreeRTOS queue
    ///
    /// @param[in] h            : FreeRTOS queue handle of the queue to delete
    void delete_queue(QueueHandle_t h)
    {
        std::scoped_lock _guard(send_mutx, receive_mutx);
        vQueueDelete(h);
    }

	/// @brief Create the FreeRTOS queue as a unique pointer with deleter
    ///
    /// @note The deleter will be called when the pointer goes out of scope
    ///
    /// @param[in] n_items      : number of items the queue can hold
    /// @param[in] item_n_bytes : number of bytes per item
    std::unique_ptr<QueueDefinition> create_queue_unique_ptr(const size_t n_items, const size_t item_n_bytes)
    {
        return { create_queue(n_items, item_n_bytes),
                    [this](auto h){ delete_queue(h); }};
    }

    StaticQueue_t queue_control_block{};                ///< FreeRTOS queue control block
    uint8_t       queue_storage[n_items * sizeof(T)]{}; ///< FreeRTOS queue storage buffer
};

} // namespace TASKMESSAGING