#ifndef QUEUE_H
#define QUEUE_H

#include "FreeRTOS.h"

typedef void *QueueHandle_t;

QueueHandle_t xQueueCreate(const UBaseType_t uxQueueLength,
                           const UBaseType_t uxItemSize);
void vQueueDelete(QueueHandle_t xQueue);
BaseType_t xQueueSend(QueueHandle_t xQueue,
                      const void *const pvItemToQueue,
                      TickType_t xTicksToWait);
BaseType_t xQueueSendToFront(QueueHandle_t xQueue,
                             const void *const pvItemToQueue,
                             TickType_t xTicksToWait);
BaseType_t xQueueReceive(QueueHandle_t xQueue,
                         void *const pvBuffer,
                         TickType_t xTicksToWait);

#endif /* QUEUE_H */
