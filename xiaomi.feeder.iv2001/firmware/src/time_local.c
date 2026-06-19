/*
 * Local civil time — spec/30-processes/time-sync.md
 */

#include "time_local.h"

#include "config_port.h"
#include "epoch_calendar.h"
#include "time_sync.h"
#include "tz_rule.h"

bool time_local_from_utc(int64_t utc_epoch, int16_t offset_min, time_local_t *out)
{
    int64_t local_epoch;
    uint8_t wday_sun0;

    if (out == NULL) {
        return false;
    }

    local_epoch = utc_epoch + (int64_t)offset_min * 60LL;
    epoch_calendar_from_epoch(local_epoch,
                              &out->year,
                              &out->month,
                              &out->day,
                              &out->hour,
                              &out->min,
                              &out->sec,
                              &wday_sun0);
    out->wday_mon0 = (uint8_t)((wday_sun0 + 6u) % 7u);
    return true;
}

bool time_local_now(time_local_t *out)
{
    int64_t utc_epoch;
    tz_rule_t rule;
    int16_t offset_min;

    if (out == NULL || !time_sync_is_valid()) {
        return false;
    }

    if (!time_sync_get_utc_epoch(&utc_epoch)) {
        return false;
    }

    if (tz_rule_load(config_port_get(), &rule) != PORT_OK) {
        tz_rule_default(&rule);
    }

    offset_min = tz_rule_effective_offset_min(&rule, utc_epoch);
    return time_local_from_utc(utc_epoch, offset_min, out);
}
