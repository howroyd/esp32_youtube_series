#pragma once

#include "Lock.h"
#include "FreeRTOS.h"
#include "task.h"

class TaskNotification 
{
public:
    TaskNotification() 
    {
        ;
    }

    bool taskNotify(TaskHandle_t taskToNotify, uint32_t value, eNotifyAction action) 
    {
	      //  BaseType_t xTaskNotify( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction )
	      return xTaskNotify(taskToNotify, value, action);
    }

#ifndef ESP_PLATFORM
    bool taskNotifyAndQuery(TaskHandle_t taskToNotify, uint32_t value, eNotifyAction action, uint32_t *previousNotifyValue) 
    {
	      //BaseType_t xTaskNotifyAndQuery( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, uint32_t *pulPreviousNotifyValue )
	      return xTaskNotifyAndQuery(taskToNotify, value, action, previousNotifyValue);
    }
#endif

    bool taskNotifyGive(TaskHandle_t taskToNotify) 
    {
	      //BaseType_t xTaskNotifyGive( TaskHandle_t xTaskToNotify )
	      return xTaskNotifyGive(taskToNotify);
    }

    bool taskNotifyWait(uint32_t bitsToClearOnEntry, uint32_t bitsToClearOnExit, uint32_t *notificationValue, TickType_t ticksToWait) 
    {
	      //BaseType_t xTaskNotifyWait( uint32_t ulBitsToClearOnEntry, uint32_t ulBitsToClearOnExit, uint32_t *pulNotificationValue, TickType_t xTicksToWait )
	      return xTaskNotifyWait(bitsToClearOnEntry, bitsToClearOnExit, notificationValue, ticksToWait);
    }

    bool taskNotifyTake(BaseType_t clearCountOnExit = pdFALSE, TickType_t ticksToWait = portMAX_DELAY)
    {
	      //uint32_t ulTaskNotifyTake( BaseType_t xClearCountOnExit, TickType_t xTicksToWait )
	      return ulTaskNotifyTake(clearCountOnExit, ticksToWait);
    }

#ifndef ESP_PLATFORM
    bool taskNotifyAndQuery_ISR(TaskHandle_t taskToNotify, uint32_t value, eNotifyAction action, uint32_t *previousNotifyValue, portBASE_TYPE& wasWoken) 
    {
	      //BaseType_t xTaskNotifyAndQueryFromISR( TaskHandle_t xTaskToNotify, uint32_t ulValue, eNotifyAction eAction, uint32_t *pulPreviousNotifyValue, BaseType_t *pxHigherPriorityTaskWoken );
	      return xTaskNotifyAndQueryFromISR(taskToNotify, value, action, previousNotifyValue, &wasWoken);
    }
#endif

    void taskNotifyGive_ISR(TaskHandle_t taskToNotify, portBASE_TYPE& waswoken) 
    {
	      //void vTaskNotifyGiveFromISR( TaskHandle_t xTaskToNotify, BaseType_t *pxHigherPriorityTaskWoken );
	      vTaskNotifyGiveFromISR(taskToNotify, &waswoken);
    }
};

// --------------------------------------------------------------------------------
// --------------------------------------------------------------------------------
