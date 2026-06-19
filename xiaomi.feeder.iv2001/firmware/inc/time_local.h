/*
 * Local civil time — spec/30-processes/time-sync.md
 */

#ifndef TIME_LOCAL_H
#define TIME_LOCAL_H

#include <stdbool.h>
#include <stdint.h>

typedef struct {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t min;
    uint8_t sec;
    uint8_t wday_mon0;
} time_local_t;

bool time_local_from_utc(int64_t utc_epoch, int16_t offset_min, time_local_t *out);
bool time_local_now(time_local_t *out);

#endif /* TIME_LOCAL_H */
