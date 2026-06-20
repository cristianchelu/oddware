/*
 * Time/TZ config apply — spec/30-processes/time-sync.md, config-store.md
 */

#include "time_config.h"

#include <string.h>

#include "mqtt_config.h"
#include "tz_rule.h"

port_err_t time_config_apply(const config_port_t *cfg, const time_config_patch_t *patch)
{
    tz_rule_t parsed;
    bool wrote = false;

    if (cfg == NULL || patch == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (patch->tz_rule_posix == NULL && patch->tz_label == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (patch->tz_rule_posix != NULL &&
        !tz_rule_parse_posix(patch->tz_rule_posix, &parsed)) {
        return PORT_ERR_INVALID_ARG;
    }

    if (patch->tz_label != NULL && strlen(patch->tz_label) >= TZ_RULE_LABEL_MAX) {
        return PORT_ERR_INVALID_ARG;
    }

    if (patch->tz_rule_posix != NULL) {
        port_err_t err = tz_rule_save_posix(cfg, patch->tz_rule_posix);

        if (err != PORT_OK) {
            return err;
        }
        wrote = true;
    }

    if (patch->tz_label != NULL) {
        port_err_t err = tz_rule_label_save(cfg, patch->tz_label);

        if (err != PORT_OK) {
            return err;
        }
        wrote = true;
    }

    if (wrote) {
        mqtt_config_publish_snapshot();
    }

    return PORT_OK;
}
