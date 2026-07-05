/*
 * WFCI bus loan adapter — spec/30-processes/wfci-bus-arbitration.md
 */

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "hal_gpio.h"
#include "hal_pinmux_define.h"

#include "adc_bus_adapter.h"
#include "board_gpio_iv2001.h"
#include "i2c_bus_adapter.h"
#include "uart2_adapter.h"
#include "wfci_bus_adapter.h"
#include "wfci_bus_port.h"
#include "wfci_bus_state.h"
#include "wfcm_bus_loan.h"

#define TM1637_CLK_GPIO  HAL_GPIO_13

static bool s_wifi_spi_active;
static bool s_loan_held;
static wfci_bus_profile_t s_held_profile;
static SemaphoreHandle_t s_arbiter;
static SemaphoreHandle_t s_loan_sem;

static struct {
    bool loan_sem;
    bool arbiter;
    bool wfcm;
} s_taken;

static void tm1637_clk_pin_init(void)
{
    hal_gpio_init(TM1637_CLK_GPIO);
    hal_pinmux_set_function(TM1637_CLK_GPIO, HAL_GPIO_13_GPIO13);
    hal_gpio_set_direction(TM1637_CLK_GPIO, HAL_GPIO_DIRECTION_OUTPUT);
    hal_gpio_set_pupd_register(TM1637_CLK_GPIO, 0, 1, 0);
    hal_gpio_set_output(TM1637_CLK_GPIO, HAL_GPIO_DATA_HIGH);
}

static void profile_hw_setup(wfci_bus_profile_t profile)
{
    switch (profile) {
    case WFCI_BUS_PROFILE_EXPANDER:
    case WFCI_BUS_PROFILE_DISPLAY:
    case WFCI_BUS_PROFILE_WEIGH:
    case WFCI_BUS_PROFILE_FULL:
        i2c_bus_adapter_init();
        break;
    case WFCI_BUS_PROFILE_ADC:
        adc_bus_adapter_init();
        break;
    default:
        break;
    }

    if (profile == WFCI_BUS_PROFILE_DISPLAY || profile == WFCI_BUS_PROFILE_FULL) {
        tm1637_clk_pin_init();
    }

    if (profile == WFCI_BUS_PROFILE_WEIGH) {
        uart2_adapter_init();
    }
}

static void profile_hw_teardown(wfci_bus_profile_t profile)
{
    switch (profile) {
    case WFCI_BUS_PROFILE_EXPANDER:
    case WFCI_BUS_PROFILE_DISPLAY:
    case WFCI_BUS_PROFILE_WEIGH:
    case WFCI_BUS_PROFILE_FULL:
        i2c_bus_adapter_deinit();
        break;
    case WFCI_BUS_PROFILE_ADC:
        adc_bus_adapter_deinit();
        break;
    default:
        break;
    }

    (void)profile;

    if (profile == WFCI_BUS_PROFILE_WEIGH) {
        uart2_adapter_deinit();
    }
}

static TickType_t wfci_acquire_ticks(uint32_t timeout_ms, bool try_only)
{
    if (try_only || timeout_ms == 0u) {
        return 0;
    }

    return pdMS_TO_TICKS(timeout_ms);
}

static TickType_t wfci_remaining_ticks(TickType_t start, TickType_t budget)
{
    TickType_t elapsed = xTaskGetTickCount() - start;

    if (elapsed >= budget) {
        return 0;
    }

    return budget - elapsed;
}

static void wfci_sync_release_partial(void)
{
    if (s_taken.wfcm) {
        wfcm_bus_loan_end();
        s_taken.wfcm = false;
    }

    if (s_taken.arbiter) {
        (void)xSemaphoreGive(s_arbiter);
        s_taken.arbiter = false;
    }

    if (s_taken.loan_sem) {
        (void)xSemaphoreGive(s_loan_sem);
        s_taken.loan_sem = false;
    }
}

static port_err_t wfci_wifi_begin(bool try_only, TickType_t arbiter_ticks, bool take_arbiter)
{
    if (!s_wifi_spi_active) {
        return PORT_OK;
    }

    if (take_arbiter) {
        if (s_arbiter == NULL || xSemaphoreTake(s_arbiter, arbiter_ticks) != pdPASS) {
            return PORT_ERR_BUSY;
        }
        s_taken.arbiter = true;
    }

    if (try_only) {
        if (!wfcm_bus_try_loan_begin()) {
            if (s_taken.arbiter) {
                (void)xSemaphoreGive(s_arbiter);
                s_taken.arbiter = false;
            }
            return PORT_ERR_BUSY;
        }
    } else if (!wfcm_bus_loan_begin()) {
        if (s_taken.arbiter) {
            (void)xSemaphoreGive(s_arbiter);
            s_taken.arbiter = false;
        }
        return PORT_ERR_IO;
    }

    s_taken.wfcm = true;
    return PORT_OK;
}

static port_err_t wfci_acquire(wfci_bus_profile_t profile,
                               wfci_bus_priority_t priority,
                               uint32_t timeout_ms,
                               bool try_only)
{
    TickType_t start;
    TickType_t budget_ticks;
    port_err_t err;

    (void)priority;

    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        if (s_loan_held) {
            return PORT_ERR_BUSY;
        }

        err = wfci_wifi_begin(try_only, 0, false);
        if (err != PORT_OK) {
            return err;
        }
        goto grant;
    }

    if (s_loan_sem == NULL) {
        return PORT_ERR_IO;
    }

    budget_ticks = wfci_acquire_ticks(timeout_ms, try_only);
    start = xTaskGetTickCount();
    if (xSemaphoreTake(s_loan_sem, budget_ticks) != pdPASS) {
        return PORT_ERR_BUSY;
    }
    s_taken.loan_sem = true;

    err = wfci_wifi_begin(try_only,
                          try_only ? 0 : wfci_remaining_ticks(start, budget_ticks),
                          true);
    if (err != PORT_OK) {
        wfci_sync_release_partial();
        return err;
    }

grant:
    profile_hw_setup(profile);
    s_held_profile = profile;
    s_loan_held = true;
    wfci_bus_state_set_held(profile, true);
    return PORT_OK;
}

static port_err_t wfci_port_acquire(wfci_bus_profile_t profile,
                                    wfci_bus_priority_t priority,
                                    uint32_t timeout_ms)
{
    return wfci_acquire(profile, priority, timeout_ms, false);
}

static port_err_t wfci_port_try_acquire(wfci_bus_profile_t profile,
                                        wfci_bus_priority_t priority)
{
    return wfci_acquire(profile, priority, 0u, true);
}

static void wfci_port_release(wfci_bus_profile_t profile)
{
    if (!s_loan_held || profile != s_held_profile) {
        return;
    }

    profile_hw_teardown(profile);
    s_loan_held = false;
    wfci_bus_state_set_held(profile, false);
    wfci_sync_release_partial();
}

static const wfci_bus_port_t s_wfci_bus = {
    .acquire = wfci_port_acquire,
    .try_acquire = wfci_port_try_acquire,
    .release = wfci_port_release,
};

void wfci_bus_sync_init(void)
{
    if (s_loan_sem == NULL) {
        s_loan_sem = xSemaphoreCreateBinary();
        if (s_loan_sem != NULL) {
            (void)xSemaphoreGive(s_loan_sem);
        }
    }

    if (s_arbiter == NULL) {
        s_arbiter = xSemaphoreCreateMutex();
    }

    wfcm_bus_sync_init();
}

void wfci_bus_wifi_spi_active(bool active)
{
    s_wifi_spi_active = active;
}

bool wfci_bus_wifi_spi_active_get(void)
{
    return s_wifi_spi_active;
}

const wfci_bus_port_t *wfci_bus_port_get(void)
{
    return &s_wfci_bus;
}
