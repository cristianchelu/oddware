/*
 * Application task and soft timers — spec/30-processes/app-event-loop.md
 */

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "app.h"
#include "app_event.h"
#include "app_event_port.h"
#include "aw9523_irq_adapter.h"
#include "button_input.h"
#include "button_port.h"
#include "hopper_input.h"
#include "hopper_ir_port.h"
#include "display_presentation.h"
#include "task_def.h"

#define APP_TASK_STACK_BYTES       4096u
#define APP_DISPLAY_TIMER_MS       50u
#define APP_HOUSEKEEPING_TIMER_MS  500u

static TaskHandle_t s_app_task;
static TimerHandle_t s_display_timer;
static TimerHandle_t s_housekeeping_timer;

static void app_post_simple(app_event_type_t type)
{
    app_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    (void)app_event_post(&ev);
}

static void app_display_timer_cb(TimerHandle_t timer)
{
    app_event_t ev;

    (void)timer;
    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_DISPLAY_TICK;
    ev.u.display_tick.now_ms =
        (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    (void)app_event_post(&ev);
}

static void app_housekeeping_timer_cb(TimerHandle_t timer)
{
    (void)timer;
    app_post_simple(EVT_TIMER_TICK);
}

static void app_timers_start(void)
{
    display_presentation_reset();

    if (s_display_timer == NULL) {
        s_display_timer = xTimerCreate("disp",
                                       pdMS_TO_TICKS(APP_DISPLAY_TIMER_MS),
                                       pdTRUE,
                                       NULL,
                                       app_display_timer_cb);
    }
    if (s_housekeeping_timer == NULL) {
        s_housekeeping_timer = xTimerCreate("app1s",
                                            pdMS_TO_TICKS(APP_HOUSEKEEPING_TIMER_MS),
                                            pdTRUE,
                                            NULL,
                                            app_housekeeping_timer_cb);
    }

    if (s_display_timer != NULL) {
        (void)xTimerStart(s_display_timer, pdMS_TO_TICKS(100));
    }
    if (s_housekeeping_timer != NULL) {
        (void)xTimerStart(s_housekeeping_timer, pdMS_TO_TICKS(100));
    }
}

static void app_dispatch_display_tick(uint32_t now_ms)
{
    app_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_DISPLAY_TICK;
    ev.u.display_tick.now_ms = now_ms;
    app_dispatch(&ev);
}

static void app_task_fn(void *param)
{
    app_event_t ev;

    (void)param;

    for (;;) {
        if (!app_event_receive(&ev, APP_DISPLAY_TIMER_MS)) {
            app_dispatch_display_tick(
                (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS));
            continue;
        }

        app_dispatch(&ev);
        app_event_release(&ev);

        while (app_event_try_receive(&ev)) {
            app_dispatch(&ev);
            app_event_release(&ev);
        }
    }
}

void app_start(void)
{
    app_event_port_init();

    if (s_app_task == NULL) {
        if (xTaskCreate(app_task_fn,
                        "app",
                        APP_TASK_STACK_BYTES / sizeof(portSTACK_TYPE),
                        NULL,
                        APP_TASK_PRIO,
                        &s_app_task) != pdPASS) {
            return;
        }
    }

    app_timers_start();
    button_input_init(button_port_get());
    hopper_input_init(hopper_ir_port_get());
    (void)aw9523_irq_adapter_start();
    app_post_simple(EVT_APP_BOOT);
}
