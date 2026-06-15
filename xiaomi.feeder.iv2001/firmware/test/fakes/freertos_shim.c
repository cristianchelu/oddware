#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>

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

void vTaskPrioritySet(TaskHandle_t task, UBaseType_t priority)
{
    (void)task;
    (void)priority;
}

UBaseType_t uxTaskPriorityGet(TaskHandle_t task)
{
    (void)task;
    return 2u;
}

void vTaskDelete(TaskHandle_t task)
{
    (void)task;
}

static uint32_t s_task_notify_count;

uint32_t ulTaskNotifyTake(BaseType_t clear_count_on_exit, TickType_t block_time)
{
    (void)block_time;

    if (clear_count_on_exit == pdTRUE && s_task_notify_count > 0u) {
        s_task_notify_count--;
    }

    return s_task_notify_count;
}

void xTaskNotifyGive(TaskHandle_t task)
{
    (void)task;
    s_task_notify_count++;
}

void *pvPortMalloc(size_t size)
{
    return malloc(size);
}

void vPortFree(void *ptr)
{
    free(ptr);
}

size_t xPortGetFreeHeapSize(void)
{
    return 192u * 1024u;
}

size_t xPortGetMinimumEverFreeHeapSize(void)
{
    return 64u * 1024u;
}
