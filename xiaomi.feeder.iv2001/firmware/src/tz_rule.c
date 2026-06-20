/*
 * POSIX TZ rule — spec/30-processes/scheduler-engine.md
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "config_keys.h"
#include "config_port.h"
#include "epoch_calendar.h"
#include "tz_rule.h"

static tz_rule_t s_cached_rule;

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

static const char *skip_space(const char *s)
{
    while (s != NULL && (*s == ' ' || *s == '\t')) {
        s++;
    }

    return s;
}

static bool is_snowflake_posix(const char *posix)
{
    size_t i;
    bool has_letter = false;

    if (posix == NULL) {
        return true;
    }

    for (i = 0; posix[i] != '\0'; i++) {
        if (isalpha((unsigned char)posix[i])) {
            has_letter = true;
            break;
        }
    }

    return !has_letter;
}

static bool has_julian_rule(const char *posix)
{
    const char *p = posix;

    if (posix == NULL) {
        return false;
    }

    while ((p = strchr(p, 'J')) != NULL) {
        if (p > posix && p[-1] == ',' && isdigit((unsigned char)p[1])) {
            return true;
        }
        p++;
    }

    return false;
}

static bool is_abbrev_only(const char *posix)
{
    size_t len;
    size_t i;

    if (posix == NULL) {
        return false;
    }

    len = strlen(posix);
    if (len < 2u || len > 5u) {
        return false;
    }

    for (i = 0; i < len; i++) {
        if (!isupper((unsigned char)posix[i])) {
            return false;
        }
    }

    return true;
}

static bool parse_posix_offset_seconds(const char **cursor, long *seconds_west)
{
    const char *c = *cursor;
    long sign = 1L;
    long hours;
    long minutes = 0L;
    long seconds = 0L;
    char *end;

    if (c == NULL || seconds_west == NULL) {
        return false;
    }

    if (*c == '+') {
        c++;
    } else if (*c == '-') {
        sign = -1L;
        c++;
    }

    if (!isdigit((unsigned char)*c)) {
        return false;
    }

    hours = strtol(c, &end, 10);
    if (end == c) {
        return false;
    }
    c = end;

    if (*c == ':') {
        c++;
        minutes = strtol(c, &end, 10);
        if (end == c) {
            return false;
        }
        c = end;
        if (*c == ':') {
            c++;
            seconds = strtol(c, &end, 10);
            if (end == c) {
                return false;
            }
            c = end;
        }
    }

    if (hours < 0L || hours > 14L || minutes < 0L || minutes > 59L ||
        seconds < 0L || seconds > 59L) {
        return false;
    }

    *seconds_west = sign * (hours * 3600L + minutes * 60L + seconds);
    *cursor = c;
    return true;
}

static bool seconds_west_to_east_min(long seconds_west, int16_t *east_min)
{
    long east_sec = -seconds_west;
    long east_minutes = east_sec / 60L;

    if (east_minutes < -720L || east_minutes > 840L) {
        return false;
    }

    *east_min = (int16_t)east_minutes;
    return true;
}

static bool parse_name(const char **cursor)
{
    const char *c = *cursor;

    if (c == NULL || !isalpha((unsigned char)*c)) {
        return false;
    }

    while (isalpha((unsigned char)*c)) {
        c++;
    }

    *cursor = c;
    return true;
}

static bool parse_m_transition(const char **cursor,
                                uint8_t *m,
                                uint8_t *w,
                                uint8_t *d,
                                uint8_t *h)
{
    unsigned long values[3];
    size_t i;
    const char *c = *cursor;
    unsigned long hour = 2u;

    if (c == NULL || *c != 'M') {
        return false;
    }

    c++;

    for (i = 0; i < 3u; i++) {
        char *end;

        values[i] = strtoul(c, &end, 10);
        if (end == c) {
            return false;
        }
        c = end;
        if (i < 2u) {
            if (*c != '.') {
                return false;
            }
            c++;
        }
    }

    if (values[0] < 1u || values[0] > 12u ||
        values[1] < 1u || values[1] > 5u ||
        values[2] > 6u) {
        return false;
    }

    if (*c == '/') {
        char *end;

        c++;
        hour = strtoul(c, &end, 10);
        if (end == c) {
            return false;
        }
        if (*end == ':') {
            return false;
        }
        c = end;
    }

    if (hour > 23u) {
        return false;
    }

    *m = (uint8_t)values[0];
    *w = (uint8_t)values[1];
    *d = (uint8_t)values[2];
    *h = (uint8_t)hour;
    *cursor = c;
    return true;
}

bool tz_rule_parse_posix(const char *posix, tz_rule_t *out)
{
    const char *cursor;
    long std_seconds_west = 0L;
    long dst_seconds_west = 0L;
    bool have_std_offset = false;
    bool have_dst_offset = false;

    if (posix == NULL || out == NULL) {
        return false;
    }

    posix = skip_space(posix);
    if (posix[0] == '\0' || strlen(posix) >= TZ_RULE_POSIX_MAX) {
        return false;
    }

    if (is_snowflake_posix(posix) || is_abbrev_only(posix)) {
        return false;
    }

    if (has_julian_rule(posix)) {
        return false;
    }

    tz_rule_default(out);
    cursor = posix;

    if (!parse_name(&cursor)) {
        return false;
    }

    if (isdigit((unsigned char)*cursor) || *cursor == '+' || *cursor == '-') {
        if (!parse_posix_offset_seconds(&cursor, &std_seconds_west)) {
            return false;
        }
        have_std_offset = true;
    }

    if (*cursor == '\0') {
        if (!have_std_offset) {
            return false;
        }
        return seconds_west_to_east_min(std_seconds_west, &out->std_offset_min) &&
               seconds_west_to_east_min(std_seconds_west, &out->dst_offset_min);
    }

    if (*cursor == ',') {
        if (!have_std_offset) {
            return false;
        }
        out->dst_offset_min = out->std_offset_min;
    } else {
        if (!parse_name(&cursor)) {
            return false;
        }

        if (isdigit((unsigned char)*cursor) || *cursor == '+' || *cursor == '-') {
            if (!parse_posix_offset_seconds(&cursor, &dst_seconds_west)) {
                return false;
            }
            have_dst_offset = true;
        }

        if (!have_std_offset ||
            !seconds_west_to_east_min(std_seconds_west, &out->std_offset_min)) {
            return false;
        }

        if (have_dst_offset) {
            if (!seconds_west_to_east_min(dst_seconds_west, &out->dst_offset_min)) {
                return false;
            }
        } else {
            out->dst_offset_min = (int16_t)(out->std_offset_min + 60);
            if (out->dst_offset_min < -720 || out->dst_offset_min > 840) {
                return false;
            }
        }

        if (*cursor == '\0') {
            return true;
        }
    }

    if (*cursor != ',') {
        return false;
    }
    cursor++;

    if (!parse_m_transition(&cursor,
                            &out->start_m,
                            &out->start_w,
                            &out->start_d,
                            &out->start_h)) {
        return false;
    }

    if (*cursor != ',') {
        return false;
    }
    cursor++;

    if (!parse_m_transition(&cursor,
                            &out->end_m,
                            &out->end_w,
                            &out->end_d,
                            &out->end_h)) {
        return false;
    }

    cursor = skip_space(cursor);
    return *cursor == '\0';
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

static void tz_rule_cache_refresh(const char *posix)
{
    tz_rule_default(&s_cached_rule);

    if (posix != NULL && posix[0] != '\0' && tz_rule_parse_posix(posix, &s_cached_rule)) {
        return;
    }
}

void tz_rule_init(void)
{
    char buf[TZ_RULE_POSIX_MAX];

    tz_rule_default(&s_cached_rule);

    if (tz_rule_load_posix(config_port_get(), buf, sizeof(buf)) == PORT_OK) {
        tz_rule_cache_refresh(buf);
    }
}

const tz_rule_t *tz_rule_get(void)
{
    return &s_cached_rule;
}

port_err_t tz_rule_load_posix(const config_port_t *cfg, char *out, size_t out_len)
{
    port_err_t err;

    if (cfg == NULL || out == NULL || out_len == 0) {
        return PORT_ERR_INVALID_ARG;
    }

    out[0] = '\0';
    err = cfg->read(CONFIG_GROUP_TIME, CONFIG_KEY_TZ_RULE, out, out_len);
    if (err == PORT_ERR_NOT_FOUND) {
        if (out_len < 5u) {
            return PORT_ERR_INVALID_ARG;
        }
        strcpy(out, "UTC0");
        return PORT_OK;
    }

    return err;
}

port_err_t tz_rule_save_posix(const config_port_t *cfg, const char *posix)
{
    tz_rule_t parsed;

    if (cfg == NULL || posix == NULL || cfg->write == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (!tz_rule_parse_posix(posix, &parsed)) {
        return PORT_ERR_INVALID_ARG;
    }

    if (strlen(posix) >= TZ_RULE_POSIX_MAX) {
        return PORT_ERR_INVALID_ARG;
    }

    if (cfg->write(CONFIG_GROUP_TIME, CONFIG_KEY_TZ_RULE, posix) != PORT_OK) {
        return PORT_ERR_IO;
    }

    tz_rule_cache_refresh(posix);
    return PORT_OK;
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

    if (label[0] == '\0') {
        port_err_t err = cfg->erase(CONFIG_GROUP_TIME, CONFIG_KEY_TZ_LABEL);

        return (err == PORT_OK || err == PORT_ERR_NOT_FOUND) ? PORT_OK : err;
    }

    return cfg->write(CONFIG_GROUP_TIME, CONFIG_KEY_TZ_LABEL, label);
}

port_err_t tz_rule_clear_posix(const config_port_t *cfg)
{
    port_err_t err;

    if (cfg == NULL || cfg->erase == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = cfg->erase(CONFIG_GROUP_TIME, CONFIG_KEY_TZ_RULE);
    if (err != PORT_OK && err != PORT_ERR_NOT_FOUND) {
        return err;
    }

    tz_rule_cache_refresh(NULL);
    return PORT_OK;
}

void tz_rule_test_reset(void)
{
    tz_rule_default(&s_cached_rule);
}
