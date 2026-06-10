/*
 * Display presentation periodic tick — spec/40-architecture/task-model.md
 */

#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "display_presentation.h"

#define DISPLAY_PRESENTATION_TIMER_MS  50u

static TimerHandle_t s_display_timer;

static void display_presentation_timer_cb(TimerHandle_t timer)
{
    uint32_t now_ms;

    (void)timer;
    now_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    (void)display_presentation_tick(now_ms);
}

void display_presentation_start(void)
{
    if (s_display_timer != NULL) {
        return;
    }

    display_presentation_reset();

    s_display_timer = xTimerCreate("disp",
                                   pdMS_TO_TICKS(DISPLAY_PRESENTATION_TIMER_MS),
                                   pdTRUE,
                                   NULL,
                                   display_presentation_timer_cb);
    if (s_display_timer != NULL) {
        (void)xTimerStart(s_display_timer, pdMS_TO_TICKS(100));
    }
}
