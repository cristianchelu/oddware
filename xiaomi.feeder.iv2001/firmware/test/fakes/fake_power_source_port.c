#include <stddef.h>

#include "fake_power_source_port.h"

static bool s_mains_present;
static port_err_t s_read_err = PORT_OK;

static port_err_t fake_power_source_read_present(bool *mains_present)
{
    if (mains_present == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (s_read_err != PORT_OK) {
        return s_read_err;
    }

    *mains_present = s_mains_present;
    return PORT_OK;
}

static const power_source_port_t s_fake_power_source = {
    .read_present = fake_power_source_read_present,
};

void fake_power_source_port_reset(void)
{
    s_mains_present = false;
    s_read_err = PORT_OK;
}

void fake_power_source_port_set_mains_present(bool mains_present)
{
    s_mains_present = mains_present;
}

void fake_power_source_port_set_read_err(port_err_t err)
{
    s_read_err = err;
}

const power_source_port_t *fake_power_source_port_get(void)
{
    return &s_fake_power_source;
}

const power_source_port_t *power_source_port_get(void)
{
    return fake_power_source_port_get();
}
