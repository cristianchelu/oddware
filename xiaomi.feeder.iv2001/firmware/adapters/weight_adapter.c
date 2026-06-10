/*
 * Weight port adapter — spec/40-architecture/ports.md
 */

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "hal_gpt.h"

#include "cs1270.h"
#include "gpio_expander_loan.h"
#include "gpio_expander_port.h"
#include "uart2_adapter.h"
#include "config_port.h"
#include "weight_driver.h"
#include "weight_port.h"
#include "wfci_bus_port.h"

static weight_driver_state_t s_state;
static bool s_weight_ready;
static bool s_boot_settling;
static TickType_t s_boot_settle_until;
static SemaphoreHandle_t s_weight_mutex;

static void weight_hal_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static port_err_t weight_uart_exchange(const uint8_t tx[CS1270_FRAME_LEN],
                                       uint8_t rx[CS1270_FRAME_LEN],
                                       uint32_t timeout_ms)
{
    return uart2_adapter_exchange(tx, rx, timeout_ms);
}

static const cs1270_uart_ops_t s_uart_ops = {
    .exchange = weight_uart_exchange,
    .delay_ms = weight_hal_delay_ms,
};

static void weight_adapter_ensure_init(void)
{
    if (s_weight_ready) {
        return;
    }

    weight_hw_t hw = {
        .expander = gpio_expander_port_get(),
        .uart = &s_uart_ops,
        .config = config_port_get(),
        .delay_ms = weight_hal_delay_ms,
    };

    weight_driver_init(&s_state, &hw);
    s_weight_ready = true;
}

static void weight_mutex_ensure(void)
{
    if (s_weight_mutex == NULL) {
        s_weight_mutex = xSemaphoreCreateMutex();
    }
}

typedef port_err_t (*weight_body_fn_t)(void);

static port_err_t weight_with_expander_loan(weight_body_fn_t fn)
{
    port_err_t err;

    weight_mutex_ensure();
    if (xSemaphoreTake(s_weight_mutex, pdMS_TO_TICKS(5000)) != pdPASS) {
        return PORT_ERR_BUSY;
    }

    err = gpio_expander_loan_begin();
    if (err != PORT_OK) {
        (void)xSemaphoreGive(s_weight_mutex);
        return err;
    }

    weight_adapter_ensure_init();
    err = fn();
    gpio_expander_loan_end();
    (void)xSemaphoreGive(s_weight_mutex);
    return err;
}

static port_err_t weight_with_uart_loan(weight_body_fn_t fn, bool try_only)
{
    const wfci_bus_port_t *bus = wfci_bus_port_get();
    port_err_t err;

    weight_mutex_ensure();
    if (xSemaphoreTake(s_weight_mutex, pdMS_TO_TICKS(5000)) != pdPASS) {
        return PORT_ERR_BUSY;
    }

    if (try_only) {
        err = bus->try_acquire(WFCI_BUS_PROFILE_WEIGH, WFCI_BUS_PRIORITY_NORMAL);
    } else {
        err = bus->acquire(WFCI_BUS_PROFILE_WEIGH, WFCI_BUS_PRIORITY_NORMAL, 5000u);
    }
    if (err != PORT_OK) {
        (void)xSemaphoreGive(s_weight_mutex);
        return err;
    }

    weight_adapter_ensure_init();
    err = fn();
    bus->release(WFCI_BUS_PROFILE_WEIGH);
    (void)xSemaphoreGive(s_weight_mutex);
    return err;
}

static port_err_t weight_with_uart(weight_body_fn_t fn)
{
    return weight_with_uart_loan(fn, false);
}

static port_err_t weight_try_with_uart(weight_body_fn_t fn)
{
    return weight_with_uart_loan(fn, true);
}

static port_err_t body_rail_enable(void)
{
    return weight_rail_enable(&s_state);
}

static port_err_t body_power_off(void)
{
    return weight_power_off(&s_state);
}

static port_err_t weight_boot_sequence_blocking(void)
{
    port_err_t err;

    weight_adapter_ensure_init();

    if (s_state.boot_done && s_state.powered && !s_state.scale_off) {
        return PORT_OK;
    }

    err = weight_with_expander_loan(body_rail_enable);
    if (err != PORT_OK) {
        return err;
    }

    /* CS1270_BOOT_MS runs with WFCI restored — see weighing.md power-on sequencing. */
    return weight_boot_settle(&s_state);
}

static port_err_t weight_ensure_booted(void)
{
    weight_adapter_ensure_init();

    if (s_state.scale_off) {
        return PORT_ERR_NOT_FOUND;
    }

    if (s_state.boot_done && s_state.powered) {
        return PORT_OK;
    }

    if (s_boot_settling) {
        return PORT_ERR_BUSY;
    }

    return weight_boot_sequence_blocking();
}

static int32_t s_read_grams;

static port_err_t body_read_grams(void)
{
    return weight_read_grams(&s_state, &s_read_grams);
}

static port_err_t body_read_raw_grams(void)
{
    return weight_read_raw_grams(&s_state, &s_read_grams);
}

static port_err_t body_cal_zero(void)
{
    return weight_calibrate_zero(&s_state);
}

static port_err_t body_cal_span(void)
{
    return weight_calibrate_span(&s_state);
}

static port_err_t port_boot_begin(void)
{
    port_err_t err;

    weight_adapter_ensure_init();
    s_state.boot_done = false;
    s_boot_settling = false;
    if (s_state.powered && !s_state.scale_off) {
        (void)weight_with_expander_loan(body_power_off);
    }

    err = weight_with_expander_loan(body_rail_enable);
    if (err != PORT_OK) {
        return err;
    }

    s_boot_settling = true;
    s_boot_settle_until = xTaskGetTickCount() + pdMS_TO_TICKS(CS1270_BOOT_MS);
    return PORT_OK;
}

static port_err_t port_boot_poll(void)
{
    weight_adapter_ensure_init();

    if (s_state.boot_done) {
        return PORT_OK;
    }

    if (!s_boot_settling || !s_state.powered || s_state.scale_off) {
        return PORT_ERR_IO;
    }

    if (xTaskGetTickCount() < s_boot_settle_until) {
        return PORT_ERR_BUSY;
    }

    s_state.boot_done = true;
    s_boot_settling = false;
    return PORT_OK;
}

static port_err_t port_power_on(void)
{
    port_err_t err;

    err = port_boot_begin();
    if (err != PORT_OK) {
        return err;
    }

    while (port_boot_poll() == PORT_ERR_BUSY) {
        vTaskDelay(pdMS_TO_TICKS(50));
    }

    return PORT_OK;
}

static port_err_t port_power_off(void)
{
    return weight_with_expander_loan(body_power_off);
}

static port_err_t port_read_grams_impl(int32_t *grams, bool try_only)
{
    port_err_t err;

    if (grams == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = weight_ensure_booted();
    if (err != PORT_OK) {
        return err;
    }

    err = try_only ? weight_try_with_uart(body_read_grams)
                   : weight_with_uart(body_read_grams);
    if (err == PORT_OK) {
        *grams = s_read_grams;
    }
    return err;
}

static port_err_t port_read_grams(int32_t *grams)
{
    return port_read_grams_impl(grams, false);
}

static port_err_t port_try_read_grams(int32_t *grams)
{
    return port_read_grams_impl(grams, true);
}

static port_err_t port_read_raw_grams(int32_t *grams)
{
    port_err_t err;

    if (grams == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = weight_ensure_booted();
    if (err != PORT_OK) {
        return err;
    }

    err = weight_with_uart(body_read_raw_grams);
    if (err == PORT_OK) {
        *grams = s_read_grams;
    }
    return err;
}

static port_err_t port_calibrate_zero(void)
{
    port_err_t err;

    err = weight_ensure_booted();
    if (err != PORT_OK) {
        return err;
    }

    return weight_with_uart(body_cal_zero);
}

static port_err_t port_calibrate_span(void)
{
    port_err_t err;

    err = weight_ensure_booted();
    if (err != PORT_OK) {
        return err;
    }

    return weight_with_uart(body_cal_span);
}

static weight_cal_status_t port_get_cal_status(void)
{
    weight_adapter_ensure_init();
    return weight_driver_cal_status(&s_state);
}

static const weight_port_t s_weight_port = {
    .boot_begin = port_boot_begin,
    .boot_poll = port_boot_poll,
    .power_on = port_power_on,
    .power_off = port_power_off,
    .read_grams = port_read_grams,
    .try_read_grams = port_try_read_grams,
    .read_raw_grams = port_read_raw_grams,
    .calibrate_zero = port_calibrate_zero,
    .calibrate_span = port_calibrate_span,
    .get_cal_status = port_get_cal_status,
};

const weight_port_t *weight_port_get(void)
{
    return &s_weight_port;
}
