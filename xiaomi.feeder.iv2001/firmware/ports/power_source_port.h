/*
 * Mains / barrel jack presence port — spec/40-architecture/ports.md
 */

#ifndef POWER_SOURCE_PORT_H
#define POWER_SOURCE_PORT_H

#include <stdbool.h>

#include "port_err.h"

typedef struct power_source_port {
    port_err_t (*read_present)(bool *mains_present);
} power_source_port_t;

const power_source_port_t *power_source_port_get(void);

#endif /* POWER_SOURCE_PORT_H */
