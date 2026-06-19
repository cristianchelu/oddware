/*
 * Compact DST timezone rule — spec/30-processes/scheduler-engine.md
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config_keys.h"
#include "epoch_calendar.h"
#include "tz_rule.h"

void tz_rule_default(tz_rule_t *out)
{
    if (out == NULL) {
        return;
    }

    memset(out, 0, sizeof(*out));
}

bool tz_rule_dst_enabled(const tz_rule_t *rule)
{
    if (rule == NULL) {
        return false;
    }

    return rule->dst_offset_min != rule->std_offset_min;
}

static bool parse_offset_field(const char **cursor, int16_t *out)
{
    long value;
    char *end;

    if (cursor == NULL || *cursor == NULL || out == NULL) {
        return false;
    }

    value = strtol(*cursor, &end, 10);
    if (end == *cursor) {
        return false;
    }

    if (value < -720L || value > 840L) {
        return false;
    }

    *out = (int16_t)value;
    *cursor = end;
    return true;
}

static bool parse_transition_field(const char **cursor,
                                   uint8_t *m,
                                   uint8_t *w,
                                   uint8_t *d,
                                   uint8_t *h)
{
    unsigned long values[4];
    size_t i;
    const char *c = *cursor;

    if (c == NULL || m == NULL || w == NULL || d == NULL || h == NULL) {
        return false;
    }

    for (i = 0; i < 4u; i++) {
        char *end;

        values[i] = strtoul(c, &end, 10);
        if (end == c) {
            return false;
        }
        c = end;
        if (i < 3u) {
            if (*c != '.') {
                return false;
            }
            c++;
        }
    }

    if (values[0] < 1u || values[0] > 12u ||
        values[1] < 1u || values[1] > 5u ||
        values[2] > 6u ||
        values[3] > 23u) {
        return false;
    }

    *m = (uint8_t)values[0];
    *w = (uint8_t)values[1];
    *d = (uint8_t)values[2];
    *h = (uint8_t)values[3];
    *cursor = c;
    return true;
}

bool tz_rule_parse_wire(const char *wire, tz_rule_t *out)
{
    const char *cursor;
    int16_t std_offset;
    int16_t dst_offset;

    if (wire == NULL || out == NULL) {
        return false;
    }

    while (*wire == ' ' || *wire == '\t') {
        wire++;
    }

    if (wire[0] == '\0') {
        return false;
    }

    tz_rule_default(out);
    cursor = wire;

    if (!parse_offset_field(&cursor, &std_offset)) {
        return false;
    }

    out->std_offset_min = std_offset;

    if (*cursor == '\0') {
        out->dst_offset_min = std_offset;
        return true;
    }

    if (*cursor != '/') {
        return false;
    }
    cursor++;

    if (!parse_offset_field(&cursor, &dst_offset)) {
        return false;
    }

    out->dst_offset_min = dst_offset;

    if (*cursor == '\0') {
        return true;
    }

    if (*cursor != '/') {
        return false;
    }
    cursor++;

    if (!parse_transition_field(&cursor,
                                &out->start_m,
                                &out->start_w,
                                &out->start_d,
                                &out->start_h)) {
        return false;
    }

    if (*cursor != '/') {
        return false;
    }
    cursor++;

    return parse_transition_field(&cursor,
                                  &out->end_m,
                                  &out->end_w,
                                  &out->end_d,
                                  &out->end_h) && *cursor == '\0';
}

bool tz_rule_format_wire(const tz_rule_t *rule, char *out, size_t out_len)
{
    int written;

    if (rule == NULL || out == NULL || out_len == 0) {
        return false;
    }

    if (!tz_rule_dst_enabled(rule)) {
        written = snprintf(out, out_len, "%d", (int)rule->std_offset_min);
        return written > 0 && (size_t)written < out_len;
    }

    written = snprintf(out,
                       out_len,
                       "%d/%d/%u.%u.%u.%u/%u.%u.%u.%u",
                       (int)rule->std_offset_min,
                       (int)rule->dst_offset_min,
                       (unsigned)rule->start_m,
                       (unsigned)rule->start_w,
                       (unsigned)rule->start_d,
                       (unsigned)rule->start_h,
                       (unsigned)rule->end_m,
                       (unsigned)rule->end_w,
                       (unsigned)rule->end_d,
                       (unsigned)rule->end_h);
    return written > 0 && (size_t)written < out_len;
}

bool tz_rule_pack(const tz_rule_t *rule, uint8_t *buf, size_t buflen)
{
    if (rule == NULL || buf == NULL || buflen < TZ_RULE_PACKED_SIZE) {
        return false;
    }

    buf[0] = (uint8_t)(rule->std_offset_min & 0xFF);
    buf[1] = (uint8_t)((rule->std_offset_min >> 8) & 0xFF);
    buf[2] = (uint8_t)(rule->dst_offset_min & 0xFF);
    buf[3] = (uint8_t)((rule->dst_offset_min >> 8) & 0xFF);
    buf[4] = rule->start_m;
    buf[5] = rule->start_w;
    buf[6] = rule->start_d;
    buf[7] = rule->start_h;
    buf[8] = rule->end_m;
    buf[9] = rule->end_w;
    buf[10] = rule->end_d;
    buf[11] = rule->end_h;
    return true;
}

bool tz_rule_unpack(const uint8_t *buf, size_t len, tz_rule_t *out)
{
    if (buf == NULL || out == NULL || len < TZ_RULE_PACKED_SIZE) {
        return false;
    }

    tz_rule_default(out);
    out->std_offset_min = (int16_t)((uint16_t)buf[0] | ((uint16_t)buf[1] << 8));
    out->dst_offset_min = (int16_t)((uint16_t)buf[2] | ((uint16_t)buf[3] << 8));
    out->start_m = buf[4];
    out->start_w = buf[5];
    out->start_d = buf[6];
    out->start_h = buf[7];
    out->end_m = buf[8];
    out->end_w = buf[9];
    out->end_d = buf[10];
    out->end_h = buf[11];
    return true;
}

static int64_t tz_transition_utc(const tz_rule_t *rule,
                                 int year,
                                 uint8_t month,
                                 uint8_t week,
                                 uint8_t dow,
                                 uint8_t hour)
{
    int day = epoch_calendar_nth_weekday(year, (int)month, (int)week, (int)dow);

    return epoch_calendar_from_ymdhms(year,
                                      (int)month,
                                      day,
                                      (int)hour,
                                      0,
                                      0,
                                      rule->std_offset_min);
}

static bool utc_in_dst_window(const tz_rule_t *rule, int64_t utc_epoch)
{
    int year;
    int64_t start_utc;
    int64_t end_utc;

    if (!tz_rule_dst_enabled(rule)) {
        return false;
    }

    for (year = 1970; year <= 2099; year++) {
        if (utc_epoch < epoch_calendar_from_ymdhms(year, 1, 1, 0, 0, 0, 0)) {
            break;
        }
    }
    year -= 1;
    if (year < 1970) {
        year = 1970;
    }

    start_utc = tz_transition_utc(rule,
                                  year,
                                  rule->start_m,
                                  rule->start_w,
                                  rule->start_d,
                                  rule->start_h);
    end_utc = tz_transition_utc(rule,
                                year,
                                rule->end_m,
                                rule->end_w,
                                rule->end_d,
                                rule->end_h);

    if (rule->start_m < rule->end_m ||
        (rule->start_m == rule->end_m &&
         (rule->start_w < rule->end_w ||
          (rule->start_w == rule->end_w && rule->start_d <= rule->end_d)))) {
        return utc_epoch >= start_utc && utc_epoch < end_utc;
    }

    return utc_epoch >= start_utc || utc_epoch < end_utc;
}

int16_t tz_rule_effective_offset_min(const tz_rule_t *rule, int64_t utc_epoch)
{
    if (rule == NULL) {
        return 0;
    }

    if (utc_in_dst_window(rule, utc_epoch)) {
        return rule->dst_offset_min;
    }

    return rule->std_offset_min;
}

port_err_t tz_rule_load(const config_port_t *cfg, tz_rule_t *out)
{
    uint8_t packed[TZ_RULE_PACKED_SIZE];
    size_t len;

    if (cfg == NULL || out == NULL || cfg->read_blob == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    tz_rule_default(out);

    if (cfg->read_blob(CONFIG_GROUP_TIME,
                       CONFIG_KEY_TZ_RULE,
                       packed,
                       sizeof(packed),
                       &len) == PORT_ERR_NOT_FOUND) {
        return PORT_OK;
    }

    if (len < TZ_RULE_PACKED_SIZE || !tz_rule_unpack(packed, len, out)) {
        tz_rule_default(out);
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

port_err_t tz_rule_save(const config_port_t *cfg, const tz_rule_t *rule)
{
    uint8_t packed[TZ_RULE_PACKED_SIZE];

    if (cfg == NULL || rule == NULL || cfg->write_blob == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (!tz_rule_pack(rule, packed, sizeof(packed))) {
        return PORT_ERR_INVALID_ARG;
    }

    return cfg->write_blob(CONFIG_GROUP_TIME,
                           CONFIG_KEY_TZ_RULE,
                           packed,
                           sizeof(packed));
}

port_err_t tz_rule_label_load(const config_port_t *cfg, char *out, size_t out_len)
{
    port_err_t err;

    if (cfg == NULL || out == NULL || out_len == 0) {
        return PORT_ERR_INVALID_ARG;
    }

    out[0] = '\0';
    err = cfg->read(CONFIG_GROUP_TIME, CONFIG_KEY_TZ_LABEL, out, out_len);
    if (err == PORT_ERR_NOT_FOUND) {
        return PORT_OK;
    }

    return err;
}

port_err_t tz_rule_label_save(const config_port_t *cfg, const char *label)
{
    if (cfg == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (label == NULL) {
        label = "";
    }

    if (strlen(label) >= TZ_RULE_LABEL_MAX) {
        return PORT_ERR_INVALID_ARG;
    }

    return cfg->write(CONFIG_GROUP_TIME, CONFIG_KEY_TZ_LABEL, label);
}
