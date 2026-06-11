/*
 * Motor port adapter — spec/40-architecture/ports.md
 */

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "gpio_expander_port.h"
#include "motor_ctrl.h"
#include "motor_driver.h"
#include "motor_port.h"

#define MOTOR_ADAPTER_MUTEX_WAIT_MS  5000u

static motor_driver_state_t s_state;
static bool s_motor_ready;
static SemaphoreHandle_t s_motor_mutex;

static void motor_hal_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static void motor_adapter_ensure_init(void)
{
    if (s_motor_ready) {
        return;
    }

    motor_hw_t hw = {
        .expander = gpio_expander_port_get(),
        .delay_ms = motor_hal_delay_ms,
    };

    motor_driver_init(&s_state, &hw);
    s_motor_ready = true;
}

static port_err_t motor_mutex_ensure(void)
{
    if (s_motor_mutex == NULL) {
        s_motor_mutex = xSemaphoreCreateMutex();
        if (s_motor_mutex == NULL) {
            return PORT_ERR_IO;
        }
    }

    return PORT_OK;
}

static port_err_t motor_port_run_locked(uint32_t duration_ms, bool reverse)
{
    port_err_t err;

    if (motor_ctrl_is_active()) {
        return PORT_ERR_BUSY;
    }

    err = motor_mutex_ensure();
    if (err != PORT_OK) {
        return err;
    }

    if (xSemaphoreTake(s_motor_mutex, pdMS_TO_TICKS(MOTOR_ADAPTER_MUTEX_WAIT_MS)) != pdPASS) {
        return PORT_ERR_BUSY;
    }

    motor_adapter_ensure_init();
    if (reverse) {
        err = motor_driver_run_reverse_ms(&s_state, duration_ms);
    } else {
        err = motor_driver_run_forward_ms(&s_state, duration_ms);
    }
    (void)xSemaphoreGive(s_motor_mutex);
    return err;
}

static port_err_t motor_port_run_forward_ms(uint32_t duration_ms)
{
    return motor_port_run_locked(duration_ms, false);
}

static port_err_t motor_port_run_reverse_ms(uint32_t duration_ms)
{
    return motor_port_run_locked(duration_ms, true);
}

static port_err_t motor_port_request_burst(uint8_t pulse_target, uint16_t timeout_ms)
{
    return motor_ctrl_request_burst(pulse_target, timeout_ms);
}

static port_err_t motor_port_request_park(uint8_t max_pulses)
{
    return motor_ctrl_request_park(max_pulses);
}

static port_err_t motor_port_stop(void)
{
    return motor_ctrl_request_stop();
}

static bool motor_port_is_active(void)
{
    return motor_ctrl_is_active();
}

static const motor_port_t s_motor_port = {
    .run_forward_ms = motor_port_run_forward_ms,
    .run_reverse_ms = motor_port_run_reverse_ms,
    .request_burst = motor_port_request_burst,
    .request_park = motor_port_request_park,
    .stop = motor_port_stop,
    .is_active = motor_port_is_active,
};

const motor_port_t *motor_port_get(void)
{
    return &s_motor_port;
}
