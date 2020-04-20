#include "main.h"

#define LOG_LOCAL_LEVEL ESP_LOG_VERBOSE
#include "esp_log.h"
#define LOG_TAG "MAIN"

//#define TOGGLE_RELAY

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
extern "C" void app_main(void)
{
	ESP_LOGD(LOG_TAG, "Creating default event loop");
	esp_event_loop_create_default();

	// Initialise the NVS
	ESP_LOGD(LOG_TAG, "Initialising NVS");
	nvs_flash_init();

	//nvs_flash_erase();

	Main main_class;

	while (!main_class.setup())
	{
		vTaskDelay(100);
	}

	while (true)
	{
		main_class.run();
	}
}
// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------

//Bt_Le::Ble& Main::ble{Bt_Le::Ble::get_instance(ESP_BT_MODE_BLE)};
//MQTT::MqttClient &Main::mqtt{MQTT::MqttClient::get_instance()};
//SNTP::Sntp& sntp{SNTP::Sntp::get_instance()};

// --------------------------------------------------------------------------------
bool Main::setup(void)
{
	esp_err_t status{ESP_OK};
	status &= relay.init();
	status &= start_all_tasks();
	return status == ESP_OK;
}

// --------------------------------------------------------------------------------
void Main::run(void)
{
#if defined(MEMORY_DEBUGGING)
	log_mem();
	vTaskDelay(pdMS_TO_TICKS(MEMORY_LOG_INTERVAL_MS));
#elif defined(TOGGLE_RELAY)
	static bool flipflop = false;
	relay.set(!relay.get());
	ESP_LOGD(LOG_TAG, "GPIO state: %s", relay.get() ? "HIGH" : "LOW");
	flipflop = !flipflop;
	vTaskDelay((10 * 60 * 1000) / portTICK_PERIOD_MS);
#else
	relay.set(true);
	vTaskDelay(portMAX_DELAY);
#endif
}

// --------------------------------------------------------------------------------
// Function to start tasks by notification
bool Main::start_all_tasks(void)
{
	bool ret_status{true};

	ble.start = true;
	//while (ble.running == false)
	//	vTaskDelay(pdMS_TO_TICKS(1000));

	//mqtt.start = true;

	return ret_status;
}

// --------------------------------------------------------------------------------
// Function to log memory to the mediator as a debug message
#if defined(MEMORY_DEBUGGING)
void Main::log_mem(void)
{
	static constexpr size_t buf_len = 255;

	// Create some buffers on the heap for the human readable strings
	char *heap_buf = new char[buf_len];
	char *stack_buf = new char[buf_len];

	// Clear the buffers to all NULL.  We're going to be using strlen and strcat.
	memset(heap_buf, 0, buf_len);
	memset(stack_buf, 0, buf_len);

	// ----------------------------------------
	// HEAP MEMORY
	multi_heap_info_t heap_info;
	heap_caps_get_info(&heap_info, MALLOC_CAP_8BIT);

#if defined(MEMORY_WARN_LOW)
	if (heap_info.minimum_free_bytes < MEMORY_HEAP_MIN ||
		heap_info.largest_free_block < MEMORY_HEAP_MIN)
#endif
#if (defined(MEMORY_VERBOSE) || defined(MEMORY_WARN_LOW))
	{
		snprintf(heap_buf, buf_len,
				 "Heap:\tSize = %uk\tFree = %uk\tLargest block = %uk\tMin = %uk",
				 (heap_info.total_allocated_bytes + heap_info.total_free_bytes) / 1024, heap_info.total_free_bytes / 1024,
				 heap_info.largest_free_block / 1024,
				 heap_info.minimum_free_bytes / 1024);
	}
#endif
	// ----------------------------------------

	// STACK MEMORY
	TaskStatus_t *pxTaskStatusArray;
	volatile UBaseType_t uxArraySize, x;
	unsigned int ulTotalRunTime;

	uxArraySize = uxTaskGetNumberOfTasks();

	// Create some memory for our FreeRTOS task list
	pxTaskStatusArray = (TaskStatus_t *)pvPortMalloc(uxArraySize * sizeof(TaskStatus_t));

	if (pxTaskStatusArray != nullptr)
	{
		// Generate raw status information about each task.
		uxArraySize = uxTaskGetSystemState(pxTaskStatusArray, uxArraySize, &ulTotalRunTime);

		// Print a header if VERBOSE or if one of our task stacks is low
#if defined(MEMORY_VERBOSE)
		snprintf(stack_buf, buf_len, "Stack min bytes:");
#elif defined(MEMORY_WARN_LOW)
		for (x = 0; x < uxArraySize; ++x)
		{
			if (pxTaskStatusArray[x].usStackHighWaterMark < MEMORY_STACK_MIN)
			{
				snprintf(stack_buf, buf_len, "Stack min bytes:");
				break;
			}
		}
#endif
		// --------------------

		// Sort; log for tasks with no core affinity first
		for (x = 0; x < uxArraySize; ++x)
		{
#if defined(MEMORY_WARN_LOW)
			if (pxTaskStatusArray[x].usStackHighWaterMark < MEMORY_STACK_MIN)
#endif
#if (defined(MEMORY_VERBOSE) || defined(MEMORY_WARN_LOW))
			{
				if (pxTaskStatusArray[x].xCoreID > 1)
				{
					snprintf(stack_buf + strlen(stack_buf), buf_len - strlen(stack_buf),
							 "\t%s = %u",
							 pxTaskStatusArray[x].pcTaskName,
							 pxTaskStatusArray[x].usStackHighWaterMark);
				}
			}
#endif
		}
		// --------------------

		// Sort; log for tasks with pinned to core 0 (PRO_CPU)
		for (x = 0; x < uxArraySize; ++x)
		{
#if defined(MEMORY_WARN_LOW)
			if (pxTaskStatusArray[x].usStackHighWaterMark < MEMORY_STACK_MIN)
#endif
#if (defined(MEMORY_VERBOSE) || defined(MEMORY_WARN_LOW))
			{
				if (pxTaskStatusArray[x].xCoreID == 0)
				{
					snprintf(stack_buf + strlen(stack_buf), buf_len - strlen(stack_buf),
							 "\t[0] %s = %u",
							 pxTaskStatusArray[x].pcTaskName,
							 pxTaskStatusArray[x].usStackHighWaterMark);
				}
			}
#endif
		}
		// --------------------

		// Sort; log for tasks with pinned to core 1 (APP_CPU)
		for (x = 0; x < uxArraySize; ++x)
		{
#if defined(MEMORY_WARN_LOW)
			if (pxTaskStatusArray[x].usStackHighWaterMark < MEMORY_STACK_MIN)
#endif
#if (defined(MEMORY_VERBOSE) || defined(MEMORY_WARN_LOW))
			{
				if (pxTaskStatusArray[x].xCoreID == 1)
				{
					snprintf(stack_buf + strlen(stack_buf), buf_len - strlen(stack_buf),
							 "\t[1] %s = %u",
							 pxTaskStatusArray[x].pcTaskName,
							 pxTaskStatusArray[x].usStackHighWaterMark);
				}
			}
#endif
		}
		// --------------------

		vPortFree(pxTaskStatusArray);
	}
	// ----------------------------------------

	// Assemble the human readable string if VERBOSE or heap is low or a stack is low
	if (strlen(heap_buf) > 0 || strlen(stack_buf) > 0)
	{
		// Send the debug message to the DEBUG console through the H7
		ESP_LOGI(LOG_TAG, "%s", heap_buf);
		ESP_LOGI(LOG_TAG, "%s", stack_buf);
	}

	delete[] heap_buf;
	delete[] stack_buf;
}
#endif