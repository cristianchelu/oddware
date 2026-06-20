/*
 * POSIX TZ rule — spec/30-processes/scheduler-engine.md
 */

#ifndef TZ_RULE_H
#define TZ_RULE_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "config_port.h"
#include "port_err.h"

#define TZ_RULE_POSIX_MAX       80u
#define TZ_RULE_LABEL_MAX       48u

typedef struct {
    int16_t std_offset_min;
    int16_t dst_offset_min;
    uint8_t start_m;
    uint8_t start_w;
    uint8_t start_d;
    uint8_t start_h;
    uint8_t end_m;
    uint8_t end_w;
    uint8_t end_d;
    uint8_t end_h;
} tz_rule_t;

void tz_rule_default(tz_rule_t *out);
bool tz_rule_dst_enabled(const tz_rule_t *rule);
bool tz_rule_parse_posix(const char *posix, tz_rule_t *out);
int16_t tz_rule_effective_offset_min(const tz_rule_t *rule, int64_t utc_epoch);

void tz_rule_init(void);
const tz_rule_t *tz_rule_get(void);
port_err_t tz_rule_save_posix(const config_port_t *cfg, const char *posix);
port_err_t tz_rule_clear_posix(const config_port_t *cfg);
port_err_t tz_rule_load_posix(const config_port_t *cfg, char *out, size_t out_len);
port_err_t tz_rule_label_load(const config_port_t *cfg, char *out, size_t out_len);
port_err_t tz_rule_label_save(const config_port_t *cfg, const char *label);

void tz_rule_test_reset(void);

#endif /* TZ_RULE_H */
