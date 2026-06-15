/*
 * Application event queue types — spec/30-processes/app-event-loop.md,
 * spec/40-architecture/task-model.md
 */

#ifndef APP_EVENT_H
#define APP_EVENT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "dispense.h"

typedef enum {
    EVT_APP_BOOT = 0,
    EVT_WIFI_STA_CONNECTING,
    EVT_WIFI_STA_READY,
    EVT_WIFI_STA_FAILED,
    EVT_WIFI_STA_AP_MODE,
    EVT_MQTT_SESSION,
    EVT_MQTT_CONNECTED,
    EVT_MQTT_MESSAGE,
    EVT_DISPLAY_TICK,
    EVT_TIMER_TICK,
    EVT_BUTTON_IRQ,
    EVT_BUTTON_GESTURE,
    EVT_DISPENSE_REQUEST,
    EVT_BURST_DONE,
    EVT_MOTOR_FAULT,
    EVT_PARK_DONE,
    EVT_TIMED_RUN_DONE,
} app_event_type_t;

typedef enum {
    MQTT_SESSION_OFF = 0,
    MQTT_SESSION_CONNECTING,
    MQTT_SESSION_CONNECTED,
    MQTT_SESSION_ERROR,
} mqtt_session_phase_t;

typedef struct {
    char ip[20];
} app_wifi_ready_t;

typedef struct {
    mqtt_session_phase_t phase;
} app_mqtt_session_t;

typedef struct {
    char *topic;
    void *payload;
    size_t len;
} app_mqtt_message_t;

typedef struct {
    uint32_t now_ms;
} app_display_tick_t;

typedef struct {
    uint32_t now_ms;
} app_button_irq_t;

typedef enum {
    APP_BUTTON_GESTURE_SHORT = 0,
    APP_BUTTON_GESTURE_LONG,
    APP_BUTTON_GESTURE_CHILD_LOCK_TOGGLE,
} app_button_gesture_kind_t;

typedef struct {
    uint8_t button_id;
    app_button_gesture_kind_t kind;
    uint32_t now_ms;
} app_button_gesture_t;

typedef struct {
    app_event_type_t type;
    union {
        app_wifi_ready_t wifi_ready;
        app_mqtt_session_t mqtt_session;
        app_mqtt_message_t mqtt_message;
        app_display_tick_t display_tick;
        app_button_irq_t button_irq;
        app_button_gesture_t button_gesture;
        app_dispense_request_t dispense_request;
    } u;
} app_event_t;

#define APP_EVENT_QUEUE_DEPTH  32u

bool app_event_post(const app_event_t *ev);
void app_event_release(app_event_t *ev);

#endif /* APP_EVENT_H */
