/*
 * Unix epoch ↔ civil calendar — spec/30-processes/time-sync.md
 */

#ifndef EPOCH_CALENDAR_H
#define EPOCH_CALENDAR_H

#include <stdbool.h>
#include <stdint.h>

bool epoch_calendar_is_leap_year(int year);
int epoch_calendar_days_in_month(int year, int month);
int epoch_calendar_weekday_sun0(int year, int month, int day);
int epoch_calendar_nth_weekday(int year, int month, int week, int dow);

int64_t epoch_calendar_from_ymdhms(int year,
                                   int month,
                                   int day,
                                   int hour,
                                   int min,
                                   int sec,
                                   int16_t offset_min);

void epoch_calendar_from_epoch(int64_t epoch,
                               uint16_t *year_out,
                               uint8_t *month_out,
                               uint8_t *day_out,
                               uint8_t *hour_out,
                               uint8_t *min_out,
                               uint8_t *sec_out,
                               uint8_t *wday_sun0_out);

#endif /* EPOCH_CALENDAR_H */
