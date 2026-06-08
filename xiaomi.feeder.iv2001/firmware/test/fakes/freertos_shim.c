#include <stddef.h>
#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "fake_time.h"

void vTaskDelay(const TickType_t ticks_to_delay)
{
    fake_time_advance_ms((uint32_t)ticks_to_delay);
}

BaseType_t xTaskCreate(TaskFunction_t pxTaskCode,
                       const char *const pcName,
                       const uint16_t usStackDepth,
                       void *const pvParameters,
                       UBaseType_t uxPriority,
                       TaskHandle_t *const pxCreatedTask)
{
    (void)pxTaskCode;
    (void)pcName;
    (void)usStackDepth;
    (void)pvParameters;
    (void)uxPriority;

    if (pxCreatedTask != NULL) {
        *pxCreatedTask = (TaskHandle_t)(uintptr_t)1;
    }

    return pdPASS;
}

TickType_t xTaskGetTickCount(void)
{
    return fake_time_ticks();
}

void vTaskSuspend(TaskHandle_t task)
{
    (void)task;
}

void vTaskResume(TaskHandle_t task)
{
    (void)task;
}
