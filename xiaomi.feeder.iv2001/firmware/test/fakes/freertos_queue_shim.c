#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "fake_time.h"

typedef struct {
    uint8_t *storage;
    UBaseType_t length;
    UBaseType_t item_size;
    UBaseType_t head;
    UBaseType_t tail;
    UBaseType_t count;
} host_queue_t;

typedef struct {
    bool taken;
} host_mutex_t;

static uint32_t s_task_notify_bits;

QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength,
                           const UBaseType_t uxItemSize)
{
    host_queue_t *q;

    if (uxQueueLength == 0u || uxItemSize == 0u) {
        return NULL;
    }

    q = calloc(1, sizeof(*q));
    if (q == NULL) {
        return NULL;
    }

    q->storage = calloc(uxQueueLength, uxItemSize);
    if (q->storage == NULL) {
        free(q);
        return NULL;
    }

    q->length = uxQueueLength;
    q->item_size = uxItemSize;
    return q;
}

void vQueueDelete(QueueHandle_t xQueue)
{
    host_queue_t *q = xQueue;

    if (q == NULL) {
        return;
    }

    free(q->storage);
    free(q);
}

BaseType_t xQueueSend(QueueHandle_t xQueue,
                      const void *const pvItemToQueue,
                      TickType_t xTicksToWait)
{
    host_queue_t *q = xQueue;
    uint8_t *slot;

    (void)xTicksToWait;

    if (q == NULL || pvItemToQueue == NULL || q->count >= q->length) {
        return pdFAIL;
    }

    slot = q->storage + (q->tail * q->item_size);
    memcpy(slot, pvItemToQueue, q->item_size);
    q->tail = (q->tail + 1u) % q->length;
    q->count++;
    return pdPASS;
}

BaseType_t xQueueSendToFront(QueueHandle_t xQueue,
                             const void *const pvItemToQueue,
                             TickType_t xTicksToWait)
{
    host_queue_t *q = xQueue;
    uint8_t *slot;

    (void)xTicksToWait;

    if (q == NULL || pvItemToQueue == NULL || q->count >= q->length) {
        return pdFAIL;
    }

    q->head = (q->head + q->length - 1u) % q->length;
    slot = q->storage + (q->head * q->item_size);
    memcpy(slot, pvItemToQueue, q->item_size);
    q->count++;
    return pdPASS;
}

BaseType_t xQueueReceive(QueueHandle_t xQueue,
                         void *const pvBuffer,
                         TickType_t xTicksToWait)
{
    host_queue_t *q = xQueue;
    const uint8_t *slot;

    if (q == NULL || pvBuffer == NULL) {
        return pdFAIL;
    }

    if (q->count == 0u) {
        if (xTicksToWait > 0u) {
            fake_time_advance_ms((uint32_t)xTicksToWait);
        }
        return pdFAIL;
    }

    slot = q->storage + (q->head * q->item_size);
    memcpy(pvBuffer, slot, q->item_size);
    q->head = (q->head + 1u) % q->length;
    q->count--;
    return pdPASS;
}

SemaphoreHandle_t xSemaphoreCreateMutex(void)
{
    return calloc(1, sizeof(host_mutex_t));
}

void vSemaphoreDelete(SemaphoreHandle_t xSemaphore)
{
    free(xSemaphore);
}

BaseType_t xSemaphoreTake(SemaphoreHandle_t xSemaphore, TickType_t xTicksToWait)
{
    host_mutex_t *m = xSemaphore;

    (void)xTicksToWait;

    if (m == NULL) {
        return pdFAIL;
    }

    if (m->taken) {
        return pdFAIL;
    }

    m->taken = true;
    return pdPASS;
}

BaseType_t xSemaphoreGive(SemaphoreHandle_t xSemaphore)
{
    host_mutex_t *m = xSemaphore;

    if (m == NULL) {
        return pdFAIL;
    }

    m->taken = false;
    return pdPASS;
}

BaseType_t xTaskNotifyWait(uint32_t ulBitsToClearOnEntry,
                           uint32_t ulBitsToClearOnExit,
                           uint32_t *pulNotificationValue,
                           TickType_t xTicksToWait)
{
    (void)ulBitsToClearOnEntry;

    if (s_task_notify_bits != 0u) {
        if (pulNotificationValue != NULL) {
            *pulNotificationValue = s_task_notify_bits;
        }
        s_task_notify_bits &= ~ulBitsToClearOnExit;
        return pdTRUE;
    }

    if (xTicksToWait > 0u) {
        fake_time_advance_ms((uint32_t)xTicksToWait);
    }

    if (pulNotificationValue != NULL) {
        *pulNotificationValue = 0u;
    }

    return pdFALSE;
}

BaseType_t xTaskNotify(TaskHandle_t xTaskHandle,
                       uint32_t ulValue,
                       uint32_t eAction)
{
    (void)xTaskHandle;
    (void)eAction;
    s_task_notify_bits |= ulValue;
    return pdPASS;
}

void freertos_notify_test_reset(void)
{
    s_task_notify_bits = 0u;
}
