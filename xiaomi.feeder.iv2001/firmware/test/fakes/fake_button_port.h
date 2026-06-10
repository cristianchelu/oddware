#ifndef FAKE_BUTTON_PORT_H
#define FAKE_BUTTON_PORT_H

#include "button_port.h"
#include "port_err.h"

void fake_button_port_reset(void);
void fake_button_port_set_sample(const button_sample_t *sample);
void fake_button_port_set_read_err(port_err_t err);
const button_port_t *fake_button_port_get(void);

#endif /* FAKE_BUTTON_PORT_H */
