/*
 * Factory-reset NVDM erase — spec/30-processes/config-store.md (Factory reset)
 */

#include "provision_reset.h"

bool provision_erase_app_groups(const config_port_t *cfg)
{
    static const char *groups[] = {
        "wifi", "mqtt", "feed", "display", "schedule",
        "time", "calib", "power", "system", NULL,
    };
    size_t i;

    if (cfg == NULL) {
        return false;
    }

    for (i = 0; groups[i] != NULL; i++) {
        if (cfg->erase_group(groups[i]) != PORT_OK) {
            return false;
        }
    }

    return true;
}
