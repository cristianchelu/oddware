/*
 * AW9523B micro-session — spec/30-processes/wfci-bus-arbitration.md
 */

#include "gpio_expander_loan.h"
#include "wfci_bus_state.h"
#include "wfci_bus_port.h"

static bool s_loan_held;
static bool s_loan_owned;

static port_err_t gpio_expander_loan_begin_common(bool try_only)
{
    port_err_t err;

    if (s_loan_held) {
        return PORT_ERR_BUSY;
    }

    if (wfci_bus_expander_accessible()) {
        s_loan_held = true;
        s_loan_owned = false;
        return PORT_OK;
    }

    if (try_only) {
        err = wfci_bus_port_get()->try_acquire(WFCI_BUS_PROFILE_EXPANDER,
                                               WFCI_BUS_PRIORITY_HIGH);
    } else {
        err = wfci_bus_port_get()->acquire(WFCI_BUS_PROFILE_EXPANDER,
                                           WFCI_BUS_PRIORITY_HIGH,
                                           5000u);
    }

    if (err == PORT_OK) {
        s_loan_held = true;
        s_loan_owned = true;
    }

    return err;
}

port_err_t gpio_expander_loan_begin(void)
{
    return gpio_expander_loan_begin_common(false);
}

port_err_t gpio_expander_loan_try_begin(void)
{
    return gpio_expander_loan_begin_common(true);
}

void gpio_expander_loan_end(void)
{
    if (!s_loan_held) {
        return;
    }

    s_loan_held = false;
    if (s_loan_owned) {
        wfci_bus_port_get()->release(WFCI_BUS_PROFILE_EXPANDER);
    }
    s_loan_owned = false;
}

bool gpio_expander_loan_is_held(void)
{
    return s_loan_held;
}
