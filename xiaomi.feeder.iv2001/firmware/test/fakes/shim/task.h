#ifndef TASK_H
#define TASK_H

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

#define taskYIELD() ((void)0)

void *pvPortMalloc(size_t size);
void vPortFree(void *ptr);

#endif /* TASK_H */
