/*
 * Feed settings in NVDM — spec/30-processes/config-store.md
 */

#include <string.h>

#include "config_keys.h"
#include "config_port.h"
#include "dispense.h"
#include "feed_config.h"

static bool parse_bool(const char *value, bool *out)
{
    if (value == NULL || out == NULL) {
        return false;
    }

    if (strcmp(value, "true") == 0 || strcmp(value, "1") == 0) {
        *out = true;
        return true;
    }

    if (strcmp(value, "false") == 0 || strcmp(value, "0") == 0) {
        *out = false;
        return true;
    }

    return false;
}

static bool feed_config_child_lock_load(bool *out)
{
    const config_port_t *cfg = config_port_get();
    char buf[8];
    port_err_t err;

    if (out == NULL) {
        return false;
    }

    if (cfg == NULL) {
        return false;
    }

    err = cfg->read(CONFIG_GROUP_FEED, CONFIG_KEY_CHILD_LOCK, buf, sizeof(buf));
    if (err == PORT_ERR_NOT_FOUND) {
        *out = false;
        return true;
    }

    if (err != PORT_OK) {
        return false;
    }

    return parse_bool(buf, out);
}

bool feed_config_child_lock_is_active(void)
{
    bool locked = false;

    if (!feed_config_child_lock_load(&locked)) {
        return false;
    }

    return locked;
}

bool feed_config_child_lock_set(bool locked)
{
    const config_port_t *cfg = config_port_get();

    if (cfg == NULL) {
        return false;
    }

    return cfg->write(CONFIG_GROUP_FEED,
                      CONFIG_KEY_CHILD_LOCK,
                      locked ? "1" : "0") == PORT_OK;
}

bool feed_config_child_lock_toggle(void)
{
    bool locked = false;

    if (!feed_config_child_lock_load(&locked)) {
        return false;
    }

    locked = !locked;
    if (!feed_config_child_lock_set(locked)) {
        return !locked;
    }

    return locked;
}

static bool feed_config_mode_parse(const char *value, dispense_mode_t *out)
{
    if (value == NULL || out == NULL) {
        return false;
    }

    if (strcmp(value, "compensated") == 0) {
        *out = DISPENSE_MODE_COMPENSATED;
        return true;
    }

    if (strcmp(value, "open_loop") == 0) {
        *out = DISPENSE_MODE_OPEN_LOOP;
        return true;
    }

    return false;
}

const char *feed_config_mode_string(dispense_mode_t mode)
{
    switch (mode) {
    case DISPENSE_MODE_COMPENSATED:
        return "compensated";
    case DISPENSE_MODE_OPEN_LOOP:
    default:
        return "open_loop";
    }
}

static bool feed_config_mode_load(dispense_mode_t *out)
{
    const config_port_t *cfg = config_port_get();
    char buf[16];
    port_err_t err;

    if (out == NULL) {
        return false;
    }

    if (cfg == NULL) {
        return false;
    }

    err = cfg->read(CONFIG_GROUP_FEED, CONFIG_KEY_FEED_MODE, buf, sizeof(buf));
    if (err == PORT_ERR_NOT_FOUND) {
        *out = DISPENSE_MODE_OPEN_LOOP;
        return true;
    }

    if (err != PORT_OK) {
        return false;
    }

    return feed_config_mode_parse(buf, out);
}

dispense_mode_t feed_config_mode_get(void)
{
    dispense_mode_t mode = DISPENSE_MODE_OPEN_LOOP;

    if (!feed_config_mode_load(&mode)) {
        return DISPENSE_MODE_OPEN_LOOP;
    }

    return mode;
}

bool feed_config_mode_set(dispense_mode_t mode)
{
    const config_port_t *cfg = config_port_get();
    const char *stored;

    if (cfg == NULL) {
        return false;
    }

    if (mode != DISPENSE_MODE_OPEN_LOOP && mode != DISPENSE_MODE_COMPENSATED) {
        return false;
    }

    stored = feed_config_mode_string(mode);
    return cfg->write(CONFIG_GROUP_FEED, CONFIG_KEY_FEED_MODE, stored) == PORT_OK;
}
