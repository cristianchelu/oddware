/*
 * Time/TZ config apply — spec/30-processes/time-sync.md, config-store.md
 */

#include "time_config.h"

#include <string.h>

#include "mqtt_config.h"
#include "mqtt_timezone.h"
#include "tz_rule.h"

#include "config_port.h"

bool time_config_format_timezone_display(char *out, size_t len)
{
    char posix[TZ_RULE_POSIX_MAX];
    char label[TZ_RULE_LABEL_MAX];
    const char *display;

    if (out == NULL || len == 0) {
        return false;
    }

    if (tz_rule_load_posix(config_port_get(), posix, sizeof(posix)) != PORT_OK) {
        strcpy(posix, "UTC0");
    }

    if (tz_rule_label_load(config_port_get(), label, sizeof(label)) != PORT_OK) {
        label[0] = '\0';
    }

    display = (label[0] != '\0') ? label : posix;
    if (strlen(display) + 1 > len) {
        return false;
    }

    strcpy(out, display);
    return true;
}

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

    if (patch->tz_rule_posix != NULL && patch->tz_rule_posix[0] != '\0' &&
        !tz_rule_parse_posix(patch->tz_rule_posix, &parsed)) {
        return PORT_ERR_INVALID_ARG;
    }

    if (patch->tz_label != NULL && patch->tz_label[0] != '\0' &&
        strlen(patch->tz_label) >= TZ_RULE_LABEL_MAX) {
        return PORT_ERR_INVALID_ARG;
    }

    if (patch->tz_rule_posix != NULL) {
        port_err_t err;

        if (patch->tz_rule_posix[0] == '\0') {
            err = tz_rule_clear_posix(cfg);
        } else {
            err = tz_rule_save_posix(cfg, patch->tz_rule_posix);
        }

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
        mqtt_timezone_publish_snapshot();
    }

    return PORT_OK;
}
