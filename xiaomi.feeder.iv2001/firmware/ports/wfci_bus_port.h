/*
 * WFCI bus loan port — spec/40-architecture/ports.md
 */

#ifndef WFCI_BUS_PORT_H
#define WFCI_BUS_PORT_H

#include <stdint.h>

#include "port_err.h"

typedef enum {
    WFCI_BUS_PROFILE_EXPANDER = 0,
    WFCI_BUS_PROFILE_DISPLAY,
    WFCI_BUS_PROFILE_ADC,
    WFCI_BUS_PROFILE_WEIGH,
    WFCI_BUS_PROFILE_FULL,
} wfci_bus_profile_t;

typedef enum {
    WFCI_BUS_PRIORITY_NORMAL = 0,
    WFCI_BUS_PRIORITY_ABOVE_NORMAL,
    WFCI_BUS_PRIORITY_HIGH,
} wfci_bus_priority_t;

typedef struct wfci_bus_port {
    port_err_t (*acquire)(wfci_bus_profile_t profile,
                          wfci_bus_priority_t priority,
                          uint32_t timeout_ms);
    port_err_t (*try_acquire)(wfci_bus_profile_t profile,
                              wfci_bus_priority_t priority);
    void (*release)(wfci_bus_profile_t profile);
} wfci_bus_port_t;

const wfci_bus_port_t *wfci_bus_port_get(void);

#endif /* WFCI_BUS_PORT_H */
