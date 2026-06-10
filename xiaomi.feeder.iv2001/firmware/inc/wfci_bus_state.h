#ifndef WFCI_BUS_STATE_H
#define WFCI_BUS_STATE_H

#include <stdbool.h>

#include "wfci_bus_port.h"

void wfci_bus_state_set_held(wfci_bus_profile_t profile, bool held);
bool wfci_bus_expander_accessible(void);

#endif /* WFCI_BUS_STATE_H */
