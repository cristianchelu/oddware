/*
 * Time/TZ config apply — spec/30-processes/time-sync.md, config-store.md
 */

#ifndef TIME_CONFIG_H
#define TIME_CONFIG_H

#include "config_port.h"
#include "port_err.h"

typedef struct {
    const char *tz_rule_posix; /* NULL = leave unchanged */
    const char *tz_label;       /* NULL = leave unchanged */
} time_config_patch_t;

port_err_t time_config_apply(const config_port_t *cfg, const time_config_patch_t *patch);

#endif /* TIME_CONFIG_H */
