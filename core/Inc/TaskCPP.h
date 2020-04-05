// REF: https://www.freertos.org/FreeRTOS_Support_Forum_Archive/July_2010/freertos_Is_it_possible_create_freertos_task_in_c_3778071.html

#pragma once

#include "FreeRTOS.h"
#include "task.h"

#include "esp_err.h"

// --------------------------------------------------------------------------------
enum TaskPriority
{
	TaskPrio_Idle = 0,														 ///< Non-Real Time operations. tasks that don't block
	TaskPrio_Lowest = (configMAX_PRIORITIES > 1),							 ///< Non-Critical back ground operations
	TaskPrio_Low = (TaskPrio_Lowest + (configMAX_PRIORITIES > 5)),			 ///< Normal Level
	TaskPrio_Mid = (configMAX_PRIORITIES / 2),								 ///< Semi-Critical, have deadlines, not a lot of processing
	TaskPrio_High = (configMAX_PRIORITIES - 1 - (configMAX_PRIORITIES > 4)), ///< Urgent tasks, short deadlines, not much processing
	TaskPrio_Highest = (configMAX_PRIORITIES - 1)							 ///< Critical Tasks, Do NOW, must be quick (Used by FreeRTOS)
};

// --------------------------------------------------------------------------------
class TaskBase
{
protected:
	TaskBase() : handle(0)
	{
	}

public:
	virtual ~TaskBase()
	{
#if INCLUDE_vTaskDelete
		if (handle)
		{
			vTaskDelete(handle);
		}
#endif
		return;
	}

	TaskHandle_t getHandle() const
	{
		return handle;
	}

#if INCLUDE_uxTaskPriorityGet
	TaskPriority priority() const
	{
		return static_cast<TaskPriority>(uxTaskPriorityGet(handle));
	}
#endif

#if INCLUDE_vTaskPrioritySet
	void priority(TaskPriority priority_)
	{
		vTaskPrioritySet(handle, priority_);
	}
#endif

#if INCLUDE_vTaskSuspend
	void suspend()
	{
		vTaskSuspend(handle);
	}

	void resume()
	{
		vTaskResume(handle);
	}
#endif

#if INCLUDE_xTaskAbortDelay
	void abortDelay()
	{
		xTaskAbortDelay(handle);
	}

#endif

#if INCLUDE_xTaskResumeFromISR

	bool resume_ISR()
	{
		return xTaskResumeFromISR(handle);
	}
#endif

	bool notify(uint32_t value, eNotifyAction act)
	{
		return xTaskNotify(handle, value, act);
	}

	bool notify_ISR(uint32_t value, eNotifyAction act, portBASE_TYPE &waswoken)
	{
		return xTaskNotifyFromISR(handle, value, act, &waswoken);
	}

#ifndef ESP_PLATFORM
	bool notify_query(uint32_t value, eNotifyAction act, uint32_t &old)
	{
		return xTaskNotifyAndQuery(handle, value, act, &old);
	}

	bool notify_query_ISR(uint32_t value, eNotifyAction act, uint32_t &old, portBASE_TYPE &waswoken)
	{
		return xTaskNotifyAndQueryFromISR(handle, value, act, &old, &waswoken);
	}
#endif

	bool give()
	{
		return xTaskNotifyGive(handle);
	}

	void give_ISR(portBASE_TYPE &waswoken)
	{
		vTaskNotifyGiveFromISR(handle, &waswoken);
	}

protected:
	TaskHandle_t handle; ///< Handle for the task we are managing.

private:
#if __cplusplus < 201101L
	TaskBase(TaskBase const &);		  ///< We are not copyable.
	void operator=(TaskBase const &); ///< We are not assignable.
#else
	TaskBase(TaskBase const &) = delete;	   ///< We are not copyable.
	void operator=(TaskBase const &) = delete; ///< We are not assignable.
#endif // __cplusplus
};

// --------------------------------------------------------------------------------
template <uint32_t stackDepth
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
		  = 0
#endif
		  >
class TaskS : public TaskBase
{
public:
	TaskS(const char *name, void (*taskfun)(void *), TaskPriority priority_, void *myParm = 0)
	{
		handle = xTaskCreateStatic(taskfun, name, stackDepth, myParm, priority_, stack, &tcb);
	}

protected:
	// used by TaskClassS to avoid needing too much complications
	TaskS(const char *name, void (*taskfun)(void *), TaskPriority priority_,
		  unsigned portSHORT stackSize_, void *myParm)
	{
		(void)stackSize_;
		handle = xTaskCreateStatic(taskfun, name, stackDepth, myParm, priority_, stack, &tcb);
	}

private:
	StaticTask_t tcb;
	StackType_t stack[stackDepth];
};

// --------------------------------------------------------------------------------
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
template <>
class TaskS<0> : public TaskBase
{
public:
	TaskS(char const *name, void (*taskfun)(void *), TaskPriority priority_,
		  unsigned portSHORT stackSize, void *myParm = 0)
	{
		xTaskCreate(taskfun, name, stackSize, myParm, priority_, &handle);
	}
};

typedef TaskS<0> Task;
#endif

// --------------------------------------------------------------------------------
template <uint32_t stackDepth>
class TaskClassS : public TaskS<stackDepth>
{
public:
	TaskClassS(char const *name, TaskPriority priority_, unsigned portSHORT stackDepth_ = 0) : TaskS<stackDepth>(name, &taskfun,
#if INCLUDE_vTaskPrioritySet
																												 ((xTaskGetSchedulerState() == taskSCHEDULER_RUNNING)
#if INCLUDE_uxTaskPriorityGet
																												  && (uxTaskPriorityGet(0) <= priority_)
#endif
																													  )
																													 ? (
#if INCLUDE_uxTaskPriorityGet
																														   (uxTaskPriorityGet(0) > TaskPrio_Idle) ? static_cast<TaskPriority>(uxTaskPriorityGet(0) - 1) :
#endif
																																								  TaskPrio_Idle)
																													 :
#endif
																													 priority_,
																												 stackDepth_, this)
#if INCLUDE_vTaskPrioritySet
																							   ,
																							   myPriority(priority_)
#endif
	{
		;
	}

	virtual ~TaskClassS() {}

	virtual void task() = 0;
	//virtual esp_err_t init(void);
	//virtual esp_err_t deinit(void);

private:
	static void taskfun(void *myParm)
	{
		TaskClassS *myTask = static_cast<TaskClassS *>(myParm);

#if INCLUDE_vTaskPrioritySet
		myTask->priority(myTask->myPriority);
#endif

		myTask->task();

		// If we get here, task has returned, delete ourselves or block indefinitely.
#if INCLUDE_vTaskDelete
		myTask->handle = 0;
		vTaskDelete(0); // Delete ourselves
#else
		while (1)
		{
			vTaskDelay(portMAX_DELAY);
		}
#endif
	}

#if INCLUDE_vTaskPrioritySet
	TaskPriority myPriority;
#endif
};

// --------------------------------------------------------------------------------
#if (configSUPPORT_DYNAMIC_ALLOCATION == 1)
typedef TaskClassS<0> TaskClass;
#endif

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------