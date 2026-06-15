#ifndef FAKE_POWER_SOURCE_PORT_H
#define FAKE_POWER_SOURCE_PORT_H

#include <stdbool.h>

#include "port_err.h"
#include "power_source_port.h"

void fake_power_source_port_reset(void);
void fake_power_source_port_set_mains_present(bool mains_present);
void fake_power_source_port_set_read_err(port_err_t err);
const power_source_port_t *fake_power_source_port_get(void);

#endif /* FAKE_POWER_SOURCE_PORT_H */
