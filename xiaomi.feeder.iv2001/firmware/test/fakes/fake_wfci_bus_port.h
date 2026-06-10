#ifndef FAKE_WFCI_BUS_PORT_H
#define FAKE_WFCI_BUS_PORT_H

#include <stddef.h>
#include <stdint.h>

#include "wfci_bus_port.h"

typedef struct {
    wfci_bus_profile_t profile;
    wfci_bus_priority_t priority;
    uint32_t hold_ms;
} fake_wfci_bus_record_t;

void fake_wfci_bus_reset(void);
const fake_wfci_bus_record_t *fake_wfci_bus_acquires(size_t *count);
size_t fake_wfci_bus_release_count(void);
uint32_t fake_wfci_bus_max_hold_ms(void);
void fake_wfci_bus_set_acquire_err(port_err_t err);
void fake_wfci_bus_set_try_acquire_err(port_err_t err);

const wfci_bus_port_t *fake_wfci_bus_port_get(void);

#endif /* FAKE_WFCI_BUS_PORT_H */
