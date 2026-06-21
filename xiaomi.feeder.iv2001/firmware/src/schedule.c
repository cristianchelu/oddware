/*
 * Feeding schedule — spec/30-processes/scheduler-engine.md
 */

#include "schedule.h"

#include <stdio.h>
#include <string.h>

#include "config_keys.h"
#include "config_port.h"
#include "schedule_nvdm.h"
#include "time_local.h"
#include "time_sync.h"

typedef struct {
    schedule_slot_config_t cfg;
    schedule_slot_runtime_t rt;
} schedule_entry_t;

static schedule_entry_t s_slots[SCHEDULE_MAX_SLOTS];
static size_t s_slot_count;
static bool s_global_enabled = true;
static bool s_today_enabled = true;
static bool s_active_valid;
static uint8_t s_active_hour;
static uint8_t s_active_min;
static uint16_t s_last_yday;
static uint8_t s_last_hour;
static uint8_t s_last_min;
static bool s_poll_clock_valid;
static schedule_changed_fn s_changed_fn;
static schedule_fire_fn s_fire_fn;

static uint16_t schedule_minutes(uint8_t hour, uint8_t min)
{
    return (uint16_t)((uint16_t)hour * 60u + (uint16_t)min);
}

static int schedule_find_index(uint8_t hour, uint8_t min)
{
    size_t i;

    for (i = 0; i < s_slot_count; i++) {
        if (s_slots[i].cfg.hour == hour && s_slots[i].cfg.min == min) {
            return (int)i;
        }
    }

    return -1;
}

static void schedule_notify_changed(void)
{
    if (s_changed_fn != NULL) {
        s_changed_fn();
    }
}

static void schedule_sort_slots(void)
{
    size_t i;

    for (i = 1; i < s_slot_count; i++) {
        schedule_entry_t key = s_slots[i];
        size_t j = i;

        while (j > 0u) {
            uint16_t prev_min =
                schedule_minutes(s_slots[j - 1u].cfg.hour, s_slots[j - 1u].cfg.min);
            uint16_t key_min = schedule_minutes(key.cfg.hour, key.cfg.min);

            if (key_min >= prev_min) {
                break;
            }

            s_slots[j] = s_slots[j - 1u];
            j--;
        }

        s_slots[j] = key;
    }
}

static void schedule_reset_runtime(void)
{
    size_t i;

    for (i = 0; i < s_slot_count; i++) {
        s_slots[i].rt.state = SCHEDULE_STATE_PENDING;
        s_slots[i].rt.skip_today = false;
        s_slots[i].rt.g_actual = -1;
        s_slots[i].rt.fired_today = false;
    }

    s_today_enabled = true;
    s_active_valid = false;
}

static bool schedule_slot_applies_today(const schedule_slot_config_t *cfg, uint8_t wday_mon0)
{
    if (cfg == NULL || cfg->days == 0u) {
        return false;
    }

    return (cfg->days & (uint8_t)(1u << wday_mon0)) != 0u;
}

static void schedule_refresh_entry_state(schedule_entry_t *entry,
                                         const time_local_t *now,
                                         bool future_today)
{
    if (entry == NULL || now == NULL) {
        return;
    }

    if (entry->rt.state == SCHEDULE_STATE_DISPENSING ||
        entry->rt.state == SCHEDULE_STATE_DISPENSED ||
        entry->rt.state == SCHEDULE_STATE_FAILED) {
        return;
    }

    if (entry->rt.skip_today) {
        if (future_today) {
            entry->rt.state = SCHEDULE_STATE_TO_BE_SKIPPED;
        } else {
            entry->rt.state = SCHEDULE_STATE_SKIPPED;
        }
        return;
    }

    if (!s_today_enabled && future_today &&
        schedule_slot_applies_today(&entry->cfg, now->wday_mon0)) {
        entry->rt.state = SCHEDULE_STATE_SKIPPED;
        return;
    }

    if (entry->rt.state == SCHEDULE_STATE_SKIPPED ||
        entry->rt.state == SCHEDULE_STATE_TO_BE_SKIPPED) {
        return;
    }

    entry->rt.state = SCHEDULE_STATE_PENDING;
}

static void schedule_init_slot_runtime(schedule_entry_t *entry)
{
    if (entry == NULL) {
        return;
    }

    entry->rt.state = SCHEDULE_STATE_PENDING;
    entry->rt.skip_today = false;
    entry->rt.g_actual = -1;
    entry->rt.fired_today = false;
}

static bool schedule_nvdm_slot_valid(const schedule_nvdm_slot_t *src)
{
    if (src == NULL) {
        return false;
    }

    return src->hour <= 23u && src->min <= 59u && src->days <= 127u &&
           src->g >= SCHEDULE_G_MIN && src->g <= SCHEDULE_G_MAX &&
           (src->flags & (uint8_t)~SCHEDULE_NVDM_SLOT_FLAG_ENABLED) == 0u;
}

static bool schedule_nvdm_config_valid(const schedule_nvdm_config_t *stored)
{
    size_t i;
    size_t j;

    if (stored == NULL) {
        return false;
    }

    if (stored->magic != SCHEDULE_NVDM_CONFIG_MAGIC ||
        stored->version != SCHEDULE_NVDM_CONFIG_VERSION ||
        stored->count > SCHEDULE_MAX_SLOTS) {
        return false;
    }

    for (i = 0; i < stored->count; i++) {
        if (!schedule_nvdm_slot_valid(&stored->slots[i])) {
            return false;
        }
    }

    for (i = 0; i < stored->count; i++) {
        for (j = i + 1u; j < stored->count; j++) {
            if (stored->slots[i].hour == stored->slots[j].hour &&
                stored->slots[i].min == stored->slots[j].min) {
                return false;
            }
        }
    }

    return true;
}

static bool schedule_load_slots_binary(const uint8_t *blob, size_t blob_len)
{
    const schedule_nvdm_config_t *stored;
    size_t i;

    if (blob == NULL || blob_len != sizeof(schedule_nvdm_config_t)) {
        return false;
    }

    stored = (const schedule_nvdm_config_t *)blob;
    if (!schedule_nvdm_config_valid(stored)) {
        return false;
    }

    memset(s_slots, 0, sizeof(s_slots));
    s_slot_count = 0;

    for (i = 0; i < stored->count; i++) {
        const schedule_nvdm_slot_t *src = &stored->slots[i];

        s_slots[i].cfg.hour = src->hour;
        s_slots[i].cfg.min = src->min;
        s_slots[i].cfg.days = src->days;
        s_slots[i].cfg.g = src->g;
        s_slots[i].cfg.enabled =
            (src->flags & SCHEDULE_NVDM_SLOT_FLAG_ENABLED) != 0u;
        schedule_init_slot_runtime(&s_slots[i]);
    }

    s_slot_count = stored->count;
    schedule_sort_slots();
    return true;
}

static void schedule_reset_slots_ram(void)
{
    memset(s_slots, 0, sizeof(s_slots));
    s_slot_count = 0;
}

static bool schedule_save_slots_blob(void)
{
    const config_port_t *cfg = config_port_get();
    schedule_nvdm_config_t stored;
    size_t i;

    if (cfg == NULL || cfg->write_blob == NULL) {
        return false;
    }

    memset(&stored, 0, sizeof(stored));
    stored.magic = SCHEDULE_NVDM_CONFIG_MAGIC;
    stored.version = SCHEDULE_NVDM_CONFIG_VERSION;
    stored.count = (uint8_t)s_slot_count;

    for (i = 0; i < s_slot_count; i++) {
        const schedule_slot_config_t *slot = &s_slots[i].cfg;

        stored.slots[i].hour = slot->hour;
        stored.slots[i].min = slot->min;
        stored.slots[i].days = slot->days;
        stored.slots[i].g = slot->g;
        stored.slots[i].flags =
            slot->enabled ? SCHEDULE_NVDM_SLOT_FLAG_ENABLED : 0u;
    }

    return cfg->write_blob(CONFIG_GROUP_SCHEDULE,
                           CONFIG_KEY_SCHEDULE_SLOTS,
                           &stored,
                           sizeof(stored)) == PORT_OK;
}

static bool schedule_save_global_enabled(void)
{
    const config_port_t *cfg = config_port_get();

    if (cfg == NULL || cfg->write == NULL) {
        return false;
    }

    return cfg->write(CONFIG_GROUP_SCHEDULE,
                      CONFIG_KEY_SCHEDULE_ENABLED,
                      s_global_enabled ? "1" : "0") == PORT_OK;
}

static bool schedule_load_slots_blob(void)
{
    const config_port_t *cfg = config_port_get();
    uint8_t blob[sizeof(schedule_nvdm_config_t)];
    size_t blob_len = 0;

    schedule_reset_slots_ram();

    if (cfg == NULL || cfg->read_blob == NULL) {
        return true;
    }

    if (cfg->read_blob(CONFIG_GROUP_SCHEDULE,
                       CONFIG_KEY_SCHEDULE_SLOTS,
                       blob,
                       sizeof(blob),
                       &blob_len)
            != PORT_OK) {
        return true;
    }

    if (blob_len == 0u) {
        return true;
    }

    if (!schedule_load_slots_binary(blob, blob_len)) {
        schedule_reset_slots_ram();
        (void)schedule_save_slots_blob();
    }

    return true;
}

static void schedule_load_global_enabled(void)
{
    const config_port_t *cfg = config_port_get();
    char value[8];

    s_global_enabled = true;

    if (cfg == NULL || cfg->read == NULL) {
        return;
    }

    if (cfg->read(CONFIG_GROUP_SCHEDULE, CONFIG_KEY_SCHEDULE_ENABLED, value, sizeof(value))
            != PORT_OK) {
        return;
    }

    s_global_enabled = (strcmp(value, "0") != 0);
}

static void schedule_mark_missed_before(uint16_t now_min, uint8_t wday_mon0)
{
    size_t i;

    for (i = 0; i < s_slot_count; i++) {
        schedule_entry_t *entry = &s_slots[i];
        uint16_t slot_min = schedule_minutes(entry->cfg.hour, entry->cfg.min);

        if (!entry->cfg.enabled || !s_global_enabled ||
            !schedule_slot_applies_today(&entry->cfg, wday_mon0)) {
            continue;
        }

        if (entry->rt.fired_today || entry->rt.skip_today) {
            continue;
        }

        if (slot_min < now_min &&
            entry->rt.state != SCHEDULE_STATE_DISPENSING &&
            entry->rt.state != SCHEDULE_STATE_DISPENSED &&
            entry->rt.state != SCHEDULE_STATE_FAILED) {
            entry->rt.state = SCHEDULE_STATE_SKIPPED;
        }
    }
}

static void schedule_try_fire_current(const time_local_t *now)
{
    size_t i;

    if (!s_global_enabled || !s_today_enabled) {
        return;
    }

    if (s_fire_fn == NULL) {
        return;
    }

    for (i = 0; i < s_slot_count; i++) {
        schedule_entry_t *entry = &s_slots[i];
        schedule_fire_result_t result;

        if (!entry->cfg.enabled || entry->rt.skip_today || entry->rt.fired_today) {
            continue;
        }

        if (!schedule_slot_applies_today(&entry->cfg, now->wday_mon0)) {
            continue;
        }

        if (entry->cfg.hour != now->hour || entry->cfg.min != now->min) {
            continue;
        }

        result = s_fire_fn(entry->cfg.hour, entry->cfg.min, entry->cfg.g);
        if (result == SCHEDULE_FIRE_OK) {
            entry->rt.fired_today = true;
            entry->rt.state = SCHEDULE_STATE_DISPENSING;
            s_active_valid = true;
            s_active_hour = entry->cfg.hour;
            s_active_min = entry->cfg.min;
            schedule_notify_changed();
        }
    }
}

const char *schedule_state_wire(schedule_state_t state)
{
    switch (state) {
    case SCHEDULE_STATE_TO_BE_SKIPPED:
        return "to_be_skipped";
    case SCHEDULE_STATE_SKIPPED:
        return "skipped";
    case SCHEDULE_STATE_DISPENSING:
        return "dispensing";
    case SCHEDULE_STATE_DISPENSED:
        return "dispensed";
    case SCHEDULE_STATE_FAILED:
        return "failed";
    case SCHEDULE_STATE_PENDING:
    default:
        return "pending";
    }
}

void schedule_init(void)
{
    schedule_test_reset();
    schedule_load_global_enabled();
    (void)schedule_load_slots_blob();
}

void schedule_set_changed_fn(schedule_changed_fn fn)
{
    s_changed_fn = fn;
}

void schedule_set_fire_fn(schedule_fire_fn fn)
{
    s_fire_fn = fn;
}

size_t schedule_slot_count(void)
{
    return s_slot_count;
}

bool schedule_get_slot(size_t index,
                       schedule_slot_config_t *cfg,
                       schedule_slot_runtime_t *rt)
{
    if (index >= s_slot_count) {
        return false;
    }

    if (cfg != NULL) {
        *cfg = s_slots[index].cfg;
    }

    if (rt != NULL) {
        *rt = s_slots[index].rt;
    }

    return true;
}

bool schedule_global_enabled(void)
{
    return s_global_enabled;
}

bool schedule_today_enabled(void)
{
    return s_today_enabled;
}

bool schedule_set_slot(const schedule_slot_config_t *cfg)
{
    int index;
    schedule_slot_config_t copy;

    if (cfg == NULL || cfg->hour > 23u || cfg->min > 59u || cfg->days > 127u ||
        cfg->g < SCHEDULE_G_MIN || cfg->g > SCHEDULE_G_MAX) {
        return false;
    }

    copy = *cfg;
    index = schedule_find_index(copy.hour, copy.min);
    if (index >= 0) {
        s_slots[index].cfg = copy;
    } else {
        if (s_slot_count >= SCHEDULE_MAX_SLOTS) {
            return false;
        }

        s_slots[s_slot_count].cfg = copy;
        schedule_init_slot_runtime(&s_slots[s_slot_count]);
        s_slot_count++;
    }

    schedule_sort_slots();
    if (!schedule_save_slots_blob()) {
        return false;
    }

    schedule_notify_changed();
    return true;
}

bool schedule_delete_slot(uint8_t hour, uint8_t min)
{
    int index = schedule_find_index(hour, min);
    size_t i;

    if (index < 0) {
        return false;
    }

    for (i = (size_t)index; i + 1u < s_slot_count; i++) {
        s_slots[i] = s_slots[i + 1u];
    }

    s_slot_count--;
    if (!schedule_save_slots_blob()) {
        return false;
    }

    schedule_notify_changed();
    return true;
}

bool schedule_toggle_slot(uint8_t hour, uint8_t min)
{
    int index = schedule_find_index(hour, min);

    if (index < 0) {
        return false;
    }

    s_slots[index].cfg.enabled = !s_slots[index].cfg.enabled;
    if (!schedule_save_slots_blob()) {
        return false;
    }

    schedule_notify_changed();
    return true;
}

bool schedule_skip_slot(uint8_t hour, uint8_t min, bool skip)
{
    int index = schedule_find_index(hour, min);

    if (index < 0) {
        return false;
    }

    s_slots[index].rt.skip_today = skip;
    schedule_notify_changed();
    return true;
}

bool schedule_set_global_enabled(bool enabled)
{
    if (s_global_enabled == enabled) {
        return true;
    }

    s_global_enabled = enabled;
    if (!schedule_save_global_enabled()) {
        return false;
    }

    schedule_notify_changed();
    return true;
}

bool schedule_set_today_enabled(bool enabled)
{
    if (s_today_enabled == enabled) {
        return true;
    }

    s_today_enabled = enabled;
    schedule_notify_changed();
    return true;
}

void schedule_poll(uint32_t now_ms)
{
    time_local_t now;
    uint16_t now_min;
    bool minute_changed;
    size_t i;

    (void)now_ms;

    if (!time_sync_is_valid() || !time_local_now(&now)) {
        return;
    }

    now_min = schedule_minutes(now.hour, now.min);

    if (!s_poll_clock_valid) {
        schedule_mark_missed_before(now_min, now.wday_mon0);
        s_last_yday = (uint16_t)now.day;
        s_last_hour = now.hour;
        s_last_min = now.min;
        s_poll_clock_valid = true;
        schedule_try_fire_current(&now);
    } else if (s_last_yday != now.day) {
        schedule_reset_runtime();
        s_last_yday = (uint16_t)now.day;
        s_last_hour = now.hour;
        s_last_min = now.min;
        schedule_notify_changed();
    } else {
        minute_changed =
            (s_last_hour != now.hour) || (s_last_min != now.min);
        if (minute_changed) {
            schedule_mark_missed_before(now_min, now.wday_mon0);
            s_last_hour = now.hour;
            s_last_min = now.min;
            schedule_try_fire_current(&now);
            schedule_notify_changed();
        }
    }

    for (i = 0; i < s_slot_count; i++) {
        uint16_t slot_min = schedule_minutes(s_slots[i].cfg.hour, s_slots[i].cfg.min);
        bool future_today = slot_min > now_min;

        schedule_refresh_entry_state(&s_slots[i], &now, future_today);
    }
}

bool schedule_active_slot(uint8_t *hour_out, uint8_t *min_out)
{
    if (!s_active_valid) {
        return false;
    }

    if (hour_out != NULL) {
        *hour_out = s_active_hour;
    }

    if (min_out != NULL) {
        *min_out = s_active_min;
    }

    return true;
}

void schedule_on_dispense_complete(const schedule_dispense_result_t *result)
{
    int index;

    if (result == NULL || !s_active_valid) {
        return;
    }

    if (result->hour != s_active_hour || result->min != s_active_min) {
        return;
    }

    index = schedule_find_index(s_active_hour, s_active_min);
    s_active_valid = false;
    if (index < 0) {
        schedule_notify_changed();
        return;
    }

    s_slots[index].rt.g_actual = (int16_t)result->grams;

    switch (result->outcome) {
    case SCHEDULE_DISPENSE_OK:
    case SCHEDULE_DISPENSE_UNDERFILL:
        s_slots[index].rt.state = SCHEDULE_STATE_DISPENSED;
        break;
    default:
        s_slots[index].rt.state = SCHEDULE_STATE_FAILED;
        break;
    }

    schedule_notify_changed();
}

static int schedule_format_repeat_days(char *buf, size_t len, uint8_t days)
{
    size_t off = 0;
    int written;
    uint8_t d;
    bool first = true;

    if (buf == NULL || len < 3u) {
        return -1;
    }

    buf[off++] = '[';

    for (d = 0; d < 7u; d++) {
        if ((days & (uint8_t)(1u << d)) == 0u) {
            continue;
        }

        written = snprintf(buf + off, len - off, "%s%u", first ? "" : ",", (unsigned)d);
        if (written <= 0 || (size_t)written >= len - off) {
            return -1;
        }

        off += (size_t)written;
        first = false;
    }

    if (off + 1u >= len) {
        return -1;
    }

    buf[off++] = ']';
    buf[off] = '\0';
    return (int)off;
}

int schedule_format_state_json(char *buf, size_t len)
{
    time_local_t now;
    bool have_now;
    size_t off = 0;
    size_t i;
    int written;

    if (buf == NULL || len == 0) {
        return -1;
    }

    have_now = time_sync_is_valid() && time_local_now(&now);

    written = snprintf(buf,
                       len,
                       "{\"enabled\":%s,\"today_enabled\":%s,\"schedule\":[",
                       s_global_enabled ? "true" : "false",
                       s_today_enabled ? "true" : "false");
    if (written <= 0 || (size_t)written >= len) {
        return -1;
    }

    off = (size_t)written;

    for (i = 0; i < s_slot_count; i++) {
        const schedule_entry_t *entry = &s_slots[i];
        uint16_t slot_min = schedule_minutes(entry->cfg.hour, entry->cfg.min);
        uint16_t now_min = 0;
        bool today = false;
        bool future_today = false;
        char repeat_buf[32];
        char g_actual_buf[24];

        if (have_now) {
            now_min = schedule_minutes(now.hour, now.min);
            today = schedule_slot_applies_today(&entry->cfg, now.wday_mon0);
            future_today = today && slot_min > now_min;
            schedule_refresh_entry_state((schedule_entry_t *)&s_slots[i], &now, future_today);
        }

        if (schedule_format_repeat_days(repeat_buf, sizeof(repeat_buf), entry->cfg.days) < 0) {
            return -1;
        }

        g_actual_buf[0] = '\0';
        if (entry->rt.g_actual >= 0) {
            written = snprintf(g_actual_buf,
                               sizeof(g_actual_buf),
                               "\"g_actual\":%d,",
                               (int)entry->rt.g_actual);
            if (written <= 0 || (size_t)written >= sizeof(g_actual_buf)) {
                return -1;
            }
        }

        written = snprintf(buf + off,
                           len - off,
                           "%s{\"time\":\"%02u:%02u\","
                           "\"repeat_days\":%s,"
                           "\"g\":%u,%s"
                           "\"enabled\":%s,\"today\":%s,"
                           "\"state\":\"%s\"}",
                           (i == 0u) ? "" : ",",
                           (unsigned)entry->cfg.hour,
                           (unsigned)entry->cfg.min,
                           repeat_buf,
                           (unsigned)entry->cfg.g,
                           g_actual_buf,
                           entry->cfg.enabled ? "true" : "false",
                           today ? "true" : "false",
                           schedule_state_wire(entry->rt.state));
        if (written <= 0 || (size_t)written >= len - off) {
            return -1;
        }

        off += (size_t)written;
    }

    if (off + 3u > len) {
        return -1;
    }

    buf[off++] = ']';
    buf[off++] = '}';
    buf[off] = '\0';
    return (int)off;
}

bool schedule_compute_next(schedule_next_t *out)
{
    time_local_t now;
    uint16_t now_min;
    size_t i;
    bool found = false;
    schedule_next_t best;

    if (out == NULL || !time_sync_is_valid() || !time_local_now(&now)) {
        return false;
    }

    now_min = schedule_minutes(now.hour, now.min);
    memset(&best, 0, sizeof(best));

    for (i = 0; i < s_slot_count; i++) {
        const schedule_entry_t *entry = &s_slots[i];
        uint16_t slot_min;
        int32_t delta;

        if (!entry->cfg.enabled || !s_global_enabled || entry->cfg.days == 0u ||
            !schedule_slot_applies_today(&entry->cfg, now.wday_mon0) ||
            entry->rt.skip_today || !s_today_enabled) {
            continue;
        }

        slot_min = schedule_minutes(entry->cfg.hour, entry->cfg.min);
        delta = (int32_t)slot_min - (int32_t)now_min;
        if (delta < 0) {
            continue;
        }

        if (!found || delta < best.in_min) {
            best.hour = entry->cfg.hour;
            best.min = entry->cfg.min;
            best.g = entry->cfg.g;
            best.in_min = delta;
            found = true;
        }
    }

    if (!found) {
        return false;
    }

    *out = best;
    return true;
}

int schedule_format_next_json(char *buf, size_t len)
{
    schedule_next_t next;
    int written;

    if (buf == NULL || len == 0 || !schedule_compute_next(&next)) {
        return -1;
    }

    written = snprintf(buf,
                       len,
                       "{\"hour\":%u,\"min\":%u,\"g\":%u,\"in_min\":%ld}",
                       (unsigned)next.hour,
                       (unsigned)next.min,
                       (unsigned)next.g,
                       (long)next.in_min);
    if (written <= 0 || (size_t)written >= len) {
        return -1;
    }

    return written;
}

void schedule_test_reset(void)
{
    memset(s_slots, 0, sizeof(s_slots));
    s_slot_count = 0;
    s_global_enabled = true;
    s_today_enabled = true;
    s_active_valid = false;
    s_poll_clock_valid = false;
    s_changed_fn = NULL;
    s_fire_fn = NULL;
}
