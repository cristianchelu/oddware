#include <stddef.h>

#include "fake_button_port.h"

static button_sample_t s_sample;
static port_err_t s_read_err = PORT_OK;

static port_err_t fake_button_read_sample(button_sample_t *out)
{
    if (out == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (s_read_err != PORT_OK) {
        return s_read_err;
    }

    *out = s_sample;
    return PORT_OK;
}

static const button_port_t s_fake_button = {
    .read_sample = fake_button_read_sample,
};

void fake_button_port_reset(void)
{
    s_sample.power_pressed = false;
    s_sample.reset_pressed = false;
    s_sample.dispense_pressed = false;
    s_read_err = PORT_OK;
}

void fake_button_port_set_sample(const button_sample_t *sample)
{
    if (sample != NULL) {
        s_sample = *sample;
    }
}

void fake_button_port_set_read_err(port_err_t err)
{
    s_read_err = err;
}

const button_port_t *fake_button_port_get(void)
{
    return &s_fake_button;
}

const button_port_t *button_port_get(void)
{
    return fake_button_port_get();
}
