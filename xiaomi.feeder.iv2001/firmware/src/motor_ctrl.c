/*
 * Motor control task — spec/30-processes/dispense-cycle.md,
 * spec/30-processes/jam-detection.md, spec/40-architecture/task-model.md
 */

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "semphr.h"
#include "task.h"

#include "adc_jam_isr_adapter.h"
#include "adc_port.h"
#include "app_event.h"
#include "gpio_expander_port.h"
#include "motor_ctrl.h"
#include "motor_driver.h"
#include "motor_index_adapter.h"
#include "motor_index_port.h"
#include "motor_jam.h"
#include "motor_limits.h"
#include "port_err.h"

typedef enum {
    MOTOR_CMD_BURST = 0,
    MOTOR_CMD_PARK,
    MOTOR_CMD_STOP,
} motor_cmd_kind_t;

typedef struct {
    motor_cmd_kind_t kind;
    uint8_t pulse_target;
    uint16_t timeout_ms;
    uint8_t max_pulses;
} motor_cmd_t;

typedef enum {
    MOTOR_PHASE_IDLE = 0,
    MOTOR_PHASE_RUN_BURST,
    MOTOR_PHASE_RUN_PARK,
    MOTOR_PHASE_ANTIJAM_REV,
    MOTOR_PHASE_ANTIJAM_WIGGLE_FWD,
    MOTOR_PHASE_ANTIJAM_WIGGLE_REV,
} motor_phase_t;

typedef enum {
    MOTOR_JAM_NONE = 0,
    MOTOR_JAM_ADC_ISR,
    MOTOR_JAM_ADC_INSTANT,
    MOTOR_JAM_ADC_SUSTAINED,
    MOTOR_JAM_INDEX_TIMEOUT,
    MOTOR_JAM_SESSION_TIMEOUT,
} motor_jam_reason_t;

typedef enum {
    MOTOR_FAULT_ANTIJAM_EXHAUSTED = 0,
    MOTOR_FAULT_SESSION_TIMEOUT,
    MOTOR_FAULT_IO,
    MOTOR_FAULT_DRIVER_STOP,
    MOTOR_FAULT_DRIVER_START,
    MOTOR_FAULT_INDEX_LED,
} motor_fault_reason_t;

static motor_driver_state_t s_driver;
static bool s_driver_ready;
static SemaphoreHandle_t s_motor_mutex;
static QueueHandle_t s_cmd_q;
static TaskHandle_t s_task;
static motor_phase_t s_phase = MOTOR_PHASE_IDLE;
static motor_cmd_t s_active_cmd;
static uint8_t s_pulses;
static bool s_prev_beam_open;
static TickType_t s_en_start_tick;
static TickType_t s_phase_end_tick;
static uint32_t s_sustained_over_ms;
static uint8_t s_antijam_retries;
static bool s_instant_jam;
static uint8_t s_io_fail_streak;
static TickType_t s_last_beam_poll_tick;
static TickType_t s_index_sample_after_tick;
static motor_jam_reason_t s_last_jam_reason;
static uint16_t s_jam_ma;
static uint32_t s_jam_sustained_ms;

static void motor_ctrl_session_teardown(void);
static void motor_ctrl_post_app_event(app_event_type_t type);

static TickType_t motor_ctrl_tick_now(void)
{
    return xTaskGetTickCount();
}

static bool motor_ctrl_tick_elapsed(TickType_t since, uint32_t ms)
{
    return (motor_ctrl_tick_now() - since) >= pdMS_TO_TICKS(ms);
}

static const char *motor_ctrl_jam_reason_label(motor_jam_reason_t reason)
{
    switch (reason) {
    case MOTOR_JAM_ADC_ISR:
        return "adc isr";
    case MOTOR_JAM_ADC_INSTANT:
        return "adc instant";
    case MOTOR_JAM_ADC_SUSTAINED:
        return "adc sustained";
    case MOTOR_JAM_INDEX_TIMEOUT:
        return "index timeout";
    case MOTOR_JAM_SESSION_TIMEOUT:
        return "session timeout";
    default:
        return "unknown";
    }
}

static const char *motor_ctrl_cmd_label(void)
{
    return (s_active_cmd.kind == MOTOR_CMD_PARK) ? "park" : "burst";
}

static void motor_ctrl_log_jam(motor_jam_reason_t reason)
{
    s_last_jam_reason = reason;

    switch (reason) {
    case MOTOR_JAM_ADC_ISR:
        printf("[motor] jam: adc isr (> %u mA)\r\n", MOTOR_JAM_INSTANT_MA);
        break;
    case MOTOR_JAM_ADC_INSTANT:
        printf("[motor] jam: adc instant %u mA (> %u mA)\r\n",
               (unsigned)s_jam_ma,
               MOTOR_JAM_INSTANT_MA);
        break;
    case MOTOR_JAM_ADC_SUSTAINED:
        printf("[motor] jam: adc sustained %u mA for %lu ms (> %u mA)\r\n",
               (unsigned)s_jam_ma,
               (unsigned long)s_jam_sustained_ms,
               MOTOR_JAM_SUSTAINED_MA);
        break;
    case MOTOR_JAM_INDEX_TIMEOUT:
        printf("[motor] jam: index timeout (0 pulses in %u ms, burst)\r\n",
               (unsigned)s_active_cmd.timeout_ms);
        break;
    case MOTOR_JAM_SESSION_TIMEOUT:
        printf("[motor] jam: session timeout (%u ms)\r\n",
               (unsigned)MOTOR_RUN_MS_MAX);
        break;
    default:
        break;
    }
}

static void motor_ctrl_finish_fault(motor_fault_reason_t reason)
{
    switch (reason) {
    case MOTOR_FAULT_ANTIJAM_EXHAUSTED:
        printf("[motor] stuck: antijam retries exhausted (last jam: %s)\r\n",
               motor_ctrl_jam_reason_label(s_last_jam_reason));
        break;
    case MOTOR_FAULT_SESSION_TIMEOUT:
        printf("[motor] stuck: session timeout (%u ms)\r\n",
               (unsigned)MOTOR_RUN_MS_MAX);
        break;
    case MOTOR_FAULT_IO:
        printf("[motor] stuck: index I/O failed %u times\r\n",
               (unsigned)s_io_fail_streak);
        break;
    case MOTOR_FAULT_DRIVER_STOP:
        printf("[motor] stuck: motor stop failed\r\n");
        break;
    case MOTOR_FAULT_DRIVER_START:
        printf("[motor] stuck: motor start failed\r\n");
        break;
    case MOTOR_FAULT_INDEX_LED:
        printf("[motor] stuck: index LED on failed\r\n");
        break;
    default:
        printf("[motor] stuck: fault\r\n");
        break;
    }

    motor_ctrl_session_teardown();
    motor_ctrl_post_app_event(EVT_MOTOR_FAULT);
}

static bool motor_ctrl_session_timed_out(TickType_t now)
{
    (void)now;

    if (s_phase == MOTOR_PHASE_IDLE) {
        return false;
    }

    return motor_ctrl_tick_elapsed(s_en_start_tick, MOTOR_RUN_MS_MAX);
}

static bool motor_ctrl_force_stop(void)
{
    if (!motor_driver_is_running(&s_driver)) {
        return true;
    }

    return motor_driver_stop(&s_driver) == PORT_OK;
}

static void motor_hal_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static void motor_ctrl_driver_ensure(void)
{
    if (s_driver_ready) {
        return;
    }

    motor_hw_t hw = {
        .expander = gpio_expander_port_get(),
        .delay_ms = motor_hal_delay_ms,
    };

    motor_driver_init(&s_driver, &hw);
    s_driver_ready = true;
}

static port_err_t motor_ctrl_mutex_take(void)
{
    if (s_motor_mutex == NULL) {
        s_motor_mutex = xSemaphoreCreateMutex();
        if (s_motor_mutex == NULL) {
            return PORT_ERR_IO;
        }
    }

    if (xSemaphoreTake(s_motor_mutex, 0) != pdPASS) {
        return PORT_ERR_BUSY;
    }

    return PORT_OK;
}

static void motor_ctrl_mutex_give(void)
{
    if (s_motor_mutex != NULL) {
        (void)xSemaphoreGive(s_motor_mutex);
    }
}

static void motor_ctrl_index_session_end(void)
{
    const motor_index_port_t *idx = motor_index_port_get();

    if (idx != NULL && idx->session_end != NULL) {
        (void)idx->session_end();
    }
}

static void motor_ctrl_post_app_event(app_event_type_t type)
{
    app_event_t ev;

    ev.type = type;
    (void)app_event_post(&ev);
}

static void motor_ctrl_session_teardown(void)
{
    adc_jam_isr_adapter_stop();
    motor_ctrl_index_session_end();
    (void)motor_index_adapter_disarm_irq();
    (void)motor_ctrl_force_stop();
    s_io_fail_streak = 0u;
    s_last_beam_poll_tick = 0u;
    s_phase = MOTOR_PHASE_IDLE;
    motor_ctrl_mutex_give();
}

static void motor_ctrl_finish_burst_done(void)
{
    if (!motor_ctrl_force_stop()) {
        motor_ctrl_finish_fault(MOTOR_FAULT_DRIVER_STOP);
        return;
    }

    motor_ctrl_session_teardown();
    motor_ctrl_post_app_event(EVT_BURST_DONE);
}

static void motor_ctrl_finish_park_done(void)
{
    if (!motor_ctrl_force_stop()) {
        motor_ctrl_finish_fault(MOTOR_FAULT_DRIVER_STOP);
        return;
    }

    motor_ctrl_session_teardown();
    motor_ctrl_post_app_event(EVT_PARK_DONE);
}

static void motor_ctrl_note_io_failure(void)
{
    if (s_io_fail_streak < 255u) {
        s_io_fail_streak++;
    }

    if (s_io_fail_streak >= 3u) {
        motor_ctrl_finish_fault(MOTOR_FAULT_IO);
    }
}

static void motor_ctrl_note_io_success(void)
{
    s_io_fail_streak = 0u;
}

static port_err_t motor_ctrl_poll_beam_open(bool *beam_open)
{
    const motor_index_port_t *idx = motor_index_port_get();

    if (idx == NULL || idx->poll == NULL || beam_open == NULL) {
        return PORT_ERR_IO;
    }

    return idx->poll(beam_open);
}

static void motor_ctrl_finish_park_already_aligned(void)
{
    printf("[motor] park: already aligned (beam open)\r\n");
    motor_ctrl_mutex_give();
    motor_ctrl_post_app_event(EVT_PARK_DONE);
}

static void motor_ctrl_schedule_index_sample(TickType_t now)
{
    TickType_t ready_at = now + pdMS_TO_TICKS(MOTOR_INDEX_IRQ_DEBOUNCE_MS);

    if (ready_at > s_index_sample_after_tick) {
        s_index_sample_after_tick = ready_at;
    }
}

static void motor_ctrl_apply_beam_sample(bool beam_open)
{
    if (beam_open && !s_prev_beam_open) {
        s_pulses++;
    }

    s_prev_beam_open = beam_open;

    if (s_phase == MOTOR_PHASE_RUN_PARK && beam_open) {
        motor_ctrl_finish_park_done();
    }
}

static void motor_ctrl_try_index_sample(TickType_t now)
{
    bool beam_open;
    port_err_t err;

    if (s_index_sample_after_tick == 0u || now < s_index_sample_after_tick) {
        return;
    }

    err = motor_ctrl_poll_beam_open(&beam_open);
    if (err == PORT_ERR_BUSY) {
        s_index_sample_after_tick = now + pdMS_TO_TICKS(MOTOR_CTRL_LOOP_SLICE_MS);
        return;
    }

    s_index_sample_after_tick = 0u;

    if (err != PORT_OK) {
        motor_ctrl_note_io_failure();
        return;
    }

    motor_ctrl_note_io_success();
    motor_ctrl_apply_beam_sample(beam_open);

    if (s_phase == MOTOR_PHASE_RUN_BURST &&
        s_pulses >= s_active_cmd.pulse_target) {
        motor_ctrl_finish_burst_done();
    }
}

static bool motor_ctrl_adc_sample(uint16_t *ma_out)
{
    const adc_port_t *adc = adc_port_get();
    uint16_t ma = 0u;

    if (adc == NULL || adc->try_read_motor_load_ma == NULL) {
        return false;
    }

    if (adc->try_read_motor_load_ma(&ma) != PORT_OK) {
        return false;
    }

    if (ma_out != NULL) {
        *ma_out = ma;
    }

    return true;
}

static bool motor_ctrl_is_jam_load(uint16_t ma)
{
    return ma > MOTOR_JAM_SUSTAINED_MA;
}

static motor_jam_reason_t motor_ctrl_detect_jam(TickType_t now)
{
    uint16_t ma = 0u;

    (void)now;

    if (s_instant_jam) {
        return MOTOR_JAM_ADC_ISR;
    }

    if (motor_ctrl_adc_sample(&ma)) {
        s_jam_ma = ma;

        if (ma > MOTOR_JAM_INSTANT_MA) {
            return MOTOR_JAM_ADC_INSTANT;
        }

        if (ma > MOTOR_JAM_SUSTAINED_MA) {
            s_sustained_over_ms += MOTOR_CTRL_LOOP_SLICE_MS;
            if (s_sustained_over_ms >= MOTOR_JAM_SUSTAINED_MS) {
                s_jam_sustained_ms = s_sustained_over_ms;
                return MOTOR_JAM_ADC_SUSTAINED;
            }
        } else {
            s_sustained_over_ms = 0u;
        }
    }

    if (s_phase == MOTOR_PHASE_RUN_BURST && s_pulses == 0u &&
        s_active_cmd.timeout_ms > 0u &&
        motor_ctrl_tick_elapsed(s_en_start_tick, s_active_cmd.timeout_ms)) {
        return MOTOR_JAM_INDEX_TIMEOUT;
    }

    return MOTOR_JAM_NONE;
}

static void motor_ctrl_begin_antijam(motor_jam_reason_t reason)
{
    (void)motor_driver_stop(&s_driver);
    s_sustained_over_ms = 0u;
    s_instant_jam = false;

    if (s_antijam_retries >= MOTOR_ANTI_JAM_MAX_RETRIES) {
        motor_ctrl_finish_fault(MOTOR_FAULT_ANTIJAM_EXHAUSTED);
        return;
    }

    motor_ctrl_log_jam(reason);
    s_antijam_retries++;
    printf("[motor] antijam: retry %u/%u reverse %u ms\r\n",
           (unsigned)s_antijam_retries,
           (unsigned)MOTOR_ANTI_JAM_MAX_RETRIES,
           (unsigned)MOTOR_ANTI_JAM_REVERSE_MS);
    s_phase = MOTOR_PHASE_ANTIJAM_REV;
    (void)motor_driver_start_reverse(&s_driver);
    s_phase_end_tick = motor_ctrl_tick_now() + pdMS_TO_TICKS(MOTOR_ANTI_JAM_REVERSE_MS);
}

static port_err_t motor_ctrl_begin_run(bool park)
{
    port_err_t err;

    motor_ctrl_driver_ensure();
    s_pulses = 0u;
    s_prev_beam_open = false;
    s_sustained_over_ms = 0u;
    s_instant_jam = false;
    s_io_fail_streak = 0u;
    s_last_beam_poll_tick = 0u;
    s_index_sample_after_tick = 0u;
    s_en_start_tick = motor_ctrl_tick_now();

    {
        const motor_index_port_t *idx = motor_index_port_get();

        if (idx == NULL || idx->session_begin == NULL ||
            idx->session_begin() != PORT_OK) {
            motor_ctrl_finish_fault(MOTOR_FAULT_INDEX_LED);
            return PORT_ERR_IO;
        }

        if (idx->poll != NULL) {
            bool beam_open = false;

            if (idx->poll(&beam_open) == PORT_OK) {
                s_prev_beam_open = beam_open;
            }
        }
    }

    if (s_task != NULL) {
        (void)motor_index_adapter_arm_irq(s_task, MOTOR_CTRL_NOTIFY_INDEX);
        adc_jam_isr_adapter_start(s_task, MOTOR_CTRL_NOTIFY_ADC_JAM);
    }

    err = motor_driver_start_forward(&s_driver);
    if (err != PORT_OK) {
        motor_ctrl_finish_fault(MOTOR_FAULT_DRIVER_START);
        return err;
    }

    s_phase = park ? MOTOR_PHASE_RUN_PARK : MOTOR_PHASE_RUN_BURST;
    return PORT_OK;
}

static void motor_ctrl_retry_active_cmd(void)
{
    if (s_active_cmd.kind == MOTOR_CMD_PARK) {
        (void)motor_ctrl_begin_run(true);
    } else {
        (void)motor_ctrl_begin_run(false);
    }
}

static void motor_ctrl_after_antijam_step(TickType_t now)
{
    uint16_t ma = 0u;
    bool still_jammed = false;

    (void)now;
    (void)motor_driver_stop(&s_driver);

    if (motor_ctrl_adc_sample(&ma) && motor_ctrl_is_jam_load(ma)) {
        still_jammed = true;
    }

    if (!still_jammed) {
        printf("[motor] antijam: load %u mA ok, resuming %s\r\n",
               (unsigned)ma,
               motor_ctrl_cmd_label());
        motor_ctrl_retry_active_cmd();
        return;
    }

    if (s_phase == MOTOR_PHASE_ANTIJAM_REV) {
        printf("[motor] antijam: load %u mA still high, wiggle forward %u ms\r\n",
               (unsigned)ma,
               (unsigned)MOTOR_ANTI_JAM_WIGGLE_MS);
        s_phase = MOTOR_PHASE_ANTIJAM_WIGGLE_FWD;
        (void)motor_driver_start_forward(&s_driver);
        s_phase_end_tick = motor_ctrl_tick_now() + pdMS_TO_TICKS(MOTOR_ANTI_JAM_WIGGLE_MS);
        return;
    }

    if (s_phase == MOTOR_PHASE_ANTIJAM_WIGGLE_FWD) {
        printf("[motor] antijam: load %u mA still high, wiggle reverse %u ms\r\n",
               (unsigned)ma,
               (unsigned)MOTOR_ANTI_JAM_WIGGLE_MS);
        s_phase = MOTOR_PHASE_ANTIJAM_WIGGLE_REV;
        (void)motor_driver_start_reverse(&s_driver);
        s_phase_end_tick = motor_ctrl_tick_now() + pdMS_TO_TICKS(MOTOR_ANTI_JAM_WIGGLE_MS);
        return;
    }

    motor_ctrl_begin_antijam(s_last_jam_reason);
}

static void motor_ctrl_poll_session_beam(TickType_t now, uint32_t interval_ms)
{
    bool beam_open;
    port_err_t err;

    if (s_last_beam_poll_tick != 0u &&
        (now - s_last_beam_poll_tick) < pdMS_TO_TICKS(interval_ms)) {
        return;
    }

    s_last_beam_poll_tick = now;

    err = motor_ctrl_poll_beam_open(&beam_open);
    if (err == PORT_ERR_BUSY) {
        return;
    }

    if (err != PORT_OK) {
        motor_ctrl_note_io_failure();
        return;
    }

    motor_ctrl_note_io_success();
    motor_ctrl_apply_beam_sample(beam_open);

    if (s_phase == MOTOR_PHASE_IDLE) {
        return;
    }

    if (s_phase == MOTOR_PHASE_RUN_BURST &&
        s_pulses >= s_active_cmd.pulse_target) {
        motor_ctrl_finish_burst_done();
        return;
    }

    if (s_phase == MOTOR_PHASE_RUN_PARK &&
        s_pulses >= s_active_cmd.max_pulses) {
        motor_ctrl_finish_park_done();
    }
}

static void motor_ctrl_poll_run(TickType_t now)
{
    uint32_t notify = 0u;

    if (motor_ctrl_session_timed_out(now)) {
        motor_ctrl_finish_fault(MOTOR_FAULT_SESSION_TIMEOUT);
        return;
    }

    {
        motor_jam_reason_t jam = motor_ctrl_detect_jam(now);

        if (jam != MOTOR_JAM_NONE) {
            motor_ctrl_begin_antijam(jam);
            return;
        }
    }

    if (s_phase == MOTOR_PHASE_RUN_BURST &&
        s_pulses >= s_active_cmd.pulse_target) {
        motor_ctrl_finish_burst_done();
        return;
    }

    motor_ctrl_try_index_sample(now);

    if (s_phase == MOTOR_PHASE_RUN_BURST) {
        motor_ctrl_poll_session_beam(now, MOTOR_BURST_BEAM_POLL_MS);
        if (s_phase == MOTOR_PHASE_IDLE) {
            return;
        }
    }

    if (s_phase == MOTOR_PHASE_RUN_PARK) {
        motor_ctrl_poll_session_beam(now, MOTOR_PARK_BEAM_POLL_MS);
        if (s_phase == MOTOR_PHASE_IDLE) {
            return;
        }
    }

    (void)xTaskNotifyWait(0u, UINT32_MAX, &notify,
                          pdMS_TO_TICKS(MOTOR_CTRL_LOOP_SLICE_MS));

    if ((notify & MOTOR_CTRL_NOTIFY_INDEX) != 0u) {
        motor_ctrl_schedule_index_sample(now);
    }

    if ((notify & MOTOR_CTRL_NOTIFY_ADC_JAM) != 0u) {
        s_instant_jam = true;
    }
}

static void motor_ctrl_poll_antijam(TickType_t now)
{
    TickType_t remain;

    if (motor_ctrl_session_timed_out(now)) {
        motor_ctrl_finish_fault(MOTOR_FAULT_SESSION_TIMEOUT);
        return;
    }

    if ((int32_t)(now - s_phase_end_tick) < 0) {
        remain = s_phase_end_tick - now;
        if (remain > pdMS_TO_TICKS(MOTOR_CTRL_LOOP_SLICE_MS)) {
            remain = pdMS_TO_TICKS(MOTOR_CTRL_LOOP_SLICE_MS);
        }
        vTaskDelay(remain);
        return;
    }

    motor_ctrl_after_antijam_step(now);
}

static void motor_ctrl_handle_cmd(const motor_cmd_t *cmd)
{
    port_err_t err;

    if (cmd == NULL) {
        return;
    }

    if (cmd->kind == MOTOR_CMD_STOP) {
        if (s_phase != MOTOR_PHASE_IDLE) {
            s_instant_jam = false;
            motor_ctrl_session_teardown();
        }
        return;
    }

    err = motor_ctrl_mutex_take();
    if (err != PORT_OK) {
        (void)xQueueSendToFront(s_cmd_q, cmd, 0);
        return;
    }

    s_active_cmd = *cmd;
    s_antijam_retries = 0u;

    if (cmd->kind == MOTOR_CMD_PARK) {
        const motor_index_port_t *idx = motor_index_port_get();
        bool beam_open = false;

        if (idx != NULL && idx->sense != NULL &&
            idx->sense(&beam_open) == PORT_OK && beam_open) {
            motor_ctrl_finish_park_already_aligned();
            return;
        }

        (void)motor_ctrl_begin_run(true);
    } else {
        (void)motor_ctrl_begin_run(false);
    }
}

static void motor_ctrl_task(void *param)
{
    motor_cmd_t cmd;

    (void)param;

    for (;;) {
        if (xQueueReceive(s_cmd_q, &cmd, pdMS_TO_TICKS(MOTOR_CTRL_LOOP_SLICE_MS)) ==
            pdPASS) {
            motor_ctrl_handle_cmd(&cmd);
        }

        if (s_phase == MOTOR_PHASE_IDLE) {
            continue;
        }

        {
            TickType_t now = motor_ctrl_tick_now();

            if (s_phase == MOTOR_PHASE_RUN_BURST ||
                s_phase == MOTOR_PHASE_RUN_PARK) {
                motor_ctrl_poll_run(now);
            } else {
                motor_ctrl_poll_antijam(now);
            }
        }
    }
}

void motor_ctrl_start(void)
{
    if (s_cmd_q == NULL) {
        s_cmd_q = xQueueCreate(MOTOR_CTRL_CMD_QUEUE_DEPTH, sizeof(motor_cmd_t));
    }

    if (s_task == NULL) {
        (void)xTaskCreate(motor_ctrl_task,
                          MOTOR_CTRL_TASK_NAME,
                          MOTOR_CTRL_TASK_STACK_BYTES / sizeof(portSTACK_TYPE),
                          NULL,
                          MOTOR_CTRL_TASK_PRIO,
                          &s_task);
    }
}

bool motor_ctrl_is_active(void)
{
    return s_phase != MOTOR_PHASE_IDLE;
}

static port_err_t motor_ctrl_enqueue(const motor_cmd_t *cmd)
{
    if (cmd == NULL || s_cmd_q == NULL) {
        return PORT_ERR_IO;
    }

    if (xQueueSend(s_cmd_q, cmd, 0) != pdPASS) {
        return PORT_ERR_BUSY;
    }

    return PORT_OK;
}

port_err_t motor_ctrl_request_burst(uint8_t pulse_target, uint16_t timeout_ms)
{
    motor_cmd_t cmd = {
        .kind = MOTOR_CMD_BURST,
        .pulse_target = pulse_target,
        .timeout_ms = timeout_ms,
        .max_pulses = 0u,
    };

    if (pulse_target == 0u || timeout_ms == 0u) {
        return PORT_ERR_INVALID_ARG;
    }

    if (motor_ctrl_is_active()) {
        return PORT_ERR_BUSY;
    }

    return motor_ctrl_enqueue(&cmd);
}

port_err_t motor_ctrl_request_park(uint8_t max_pulses)
{
    motor_cmd_t cmd = {
        .kind = MOTOR_CMD_PARK,
        .pulse_target = 0u,
        .timeout_ms = 0u,
        .max_pulses = max_pulses,
    };

    if (max_pulses == 0u) {
        return PORT_ERR_INVALID_ARG;
    }

    if (motor_ctrl_is_active()) {
        return PORT_ERR_BUSY;
    }

    return motor_ctrl_enqueue(&cmd);
}

port_err_t motor_ctrl_request_stop(void)
{
    motor_cmd_t cmd = {
        .kind = MOTOR_CMD_STOP,
    };

    return motor_ctrl_enqueue(&cmd);
}

void motor_ctrl_test_reset(void)
{
    motor_cmd_t drain;

    s_phase = MOTOR_PHASE_IDLE;
    s_antijam_retries = 0u;
    s_instant_jam = false;
    s_sustained_over_ms = 0u;
    s_pulses = 0u;

    if (s_cmd_q != NULL) {
        while (xQueueReceive(s_cmd_q, &drain, 0) == pdPASS) {
        }
    }

    if (s_motor_mutex != NULL) {
        (void)xSemaphoreTake(s_motor_mutex, 0);
        (void)xSemaphoreGive(s_motor_mutex);
    }

    motor_ctrl_driver_ensure();
    (void)motor_driver_stop(&s_driver);
    motor_ctrl_index_session_end();
}

void motor_ctrl_test_notify(uint32_t bits)
{
    if (s_task != NULL) {
        (void)xTaskNotify(s_task, bits, eSetBits);
    }
}

void motor_ctrl_test_poll(void)
{
    motor_cmd_t cmd;

    if (s_cmd_q != NULL &&
        xQueueReceive(s_cmd_q, &cmd, 0) == pdPASS) {
        motor_ctrl_handle_cmd(&cmd);
    }

    if (s_phase == MOTOR_PHASE_IDLE) {
        return;
    }

    {
        TickType_t now = motor_ctrl_tick_now();

        if (s_phase == MOTOR_PHASE_RUN_BURST ||
            s_phase == MOTOR_PHASE_RUN_PARK) {
            motor_ctrl_poll_run(now);
        } else {
            motor_ctrl_poll_antijam(now);
        }
    }
}
