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

#endif /* TASK_H */
