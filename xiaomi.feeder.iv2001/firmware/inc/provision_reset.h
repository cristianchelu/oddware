/*
 * Factory-reset NVDM erase — spec/30-processes/config-store.md (Factory reset)
 */

#ifndef PROVISION_RESET_H
#define PROVISION_RESET_H

#include <stdbool.h>

#include "config_port.h"

bool provision_erase_app_groups(const config_port_t *cfg);

#endif /* PROVISION_RESET_H */
