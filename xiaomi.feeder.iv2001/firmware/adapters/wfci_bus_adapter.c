/*
 * WFCI bus loan adapter — spec/30-processes/wfci-bus-arbitration.md
 */

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "hal_gpio.h"
#include "hal_pinmux_define.h"

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
    default:
        break;
    }

    (void)profile;

    if (profile == WFCI_BUS_PROFILE_WEIGH) {
        uart2_adapter_deinit();
    }
}

static void arbiter_ensure(void)
{
    if (s_arbiter == NULL) {
        s_arbiter = xSemaphoreCreateMutex();
    }
}

static port_err_t wfci_acquire(wfci_bus_profile_t profile,
                               wfci_bus_priority_t priority,
                               uint32_t timeout_ms,
                               bool try_only)
{
    TickType_t ticks;
    port_err_t err = PORT_OK;

    (void)priority;

    if (s_loan_held) {
        return PORT_ERR_BUSY;
    }

    if (!s_wifi_spi_active) {
        profile_hw_setup(profile);
        s_held_profile = profile;
        s_loan_held = true;
        wfci_bus_state_set_held(profile, true);
        return PORT_OK;
    }

    arbiter_ensure();
    ticks = (timeout_ms == 0u) ? 0 : pdMS_TO_TICKS(timeout_ms);
    if (xSemaphoreTake(s_arbiter, ticks) != pdPASS) {
        return PORT_ERR_BUSY;
    }

    if (try_only) {
        if (!wfcm_bus_try_loan_begin()) {
            (void)xSemaphoreGive(s_arbiter);
            return PORT_ERR_BUSY;
        }
    } else {
        wfcm_bus_loan_begin();
    }

    profile_hw_setup(profile);
    s_held_profile = profile;
    s_loan_held = true;
    wfci_bus_state_set_held(profile, true);
    return err;
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

    if (s_wifi_spi_active) {
        wfcm_bus_loan_end();
        if (s_arbiter != NULL) {
            (void)xSemaphoreGive(s_arbiter);
        }
    }
}

static const wfci_bus_port_t s_wfci_bus = {
    .acquire = wfci_port_acquire,
    .try_acquire = wfci_port_try_acquire,
    .release = wfci_port_release,
};

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
