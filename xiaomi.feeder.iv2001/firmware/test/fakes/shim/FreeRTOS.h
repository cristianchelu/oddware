#ifndef FREERTOS_H
#define FREERTOS_H

#include <stdint.h>

typedef uint32_t TickType_t;
typedef void *TaskHandle_t;
typedef void (*TaskFunction_t)(void *);
typedef long BaseType_t;
typedef unsigned long UBaseType_t;
typedef long portSTACK_TYPE;

#define pdFALSE 0
#define pdTRUE  1
#define pdPASS  1
#define pdFAIL  0

#define portTICK_PERIOD_MS 1
#define pdMS_TO_TICKS(ms) ((TickType_t)(ms))
#define portMAX_DELAY      ((TickType_t)0xffffffffUL)

#endif /* FREERTOS_H */
