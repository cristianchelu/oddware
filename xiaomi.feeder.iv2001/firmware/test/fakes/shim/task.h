#ifndef TASK_H
#define TASK_H

#include <stddef.h>

#include "FreeRTOS.h"

void vTaskDelay(const TickType_t ticks_to_delay);
BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                       const char *const pcName,
                       const uint16_t usStackDepth,
                       void *const pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *const pxCreatedTask);
TickType_t xTaskGetTickCount(void);
void vTaskSuspend(TaskHandle_t task);
void vTaskResume(TaskHandle_t task);
void vTaskPrioritySet(TaskHandle_t task, UBaseType_t priority);
UBaseType_t uxTaskPriorityGet(TaskHandle_t task);
void vTaskDelete(TaskHandle_t task);

BaseType_t xTaskNotifyWait(uint32_t ulBitsToClearOnEntry,
                           uint32_t ulBitsToClearOnExit,
                           uint32_t *pulNotificationValue,
                           TickType_t xTicksToWait);
BaseType_t xTaskNotify(TaskHandle_t xTaskHandle,
                       uint32_t ulValue,
                       uint32_t eAction);

#define eSetBits 1u

#define taskYIELD() ((void)0)

void *pvPortMalloc(size_t size);
void vPortFree(void *ptr);

#endif /* TASK_H */
