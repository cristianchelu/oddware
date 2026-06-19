/*
 * Unix epoch ↔ civil calendar — spec/30-processes/time-sync.md
 */

#include <stddef.h>

#include "epoch_calendar.h"

bool epoch_calendar_is_leap_year(int year)
{
    return (year % 4 == 0 && year % 100 != 0) || (year % 400 == 0);
}

int epoch_calendar_days_in_month(int year, int month)
{
    static const int days[] = { 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 };

    if (month < 1 || month > 12) {
        return 30;
    }

    if (month == 2 && epoch_calendar_is_leap_year(year)) {
        return 29;
    }

    return days[month - 1];
}

int epoch_calendar_weekday_sun0(int year, int month, int day)
{
    int y = year;
    int m = month;
    int k;

    if (m < 3) {
        m += 12;
        y -= 1;
    }

    k = y % 100;
    return (day + (13 * (m + 1)) / 5 + k + k / 4 + (y / 100) / 4 + 5 * (y / 100)) % 7;
}

int epoch_calendar_nth_weekday(int year, int month, int week, int dow)
{
    int dim = epoch_calendar_days_in_month(year, month);
    int first_wday = epoch_calendar_weekday_sun0(year, month, 1);
    int day;

    if (week == 5) {
        day = dim;
        while (epoch_calendar_weekday_sun0(year, month, day) != dow) {
            day--;
        }
        return day;
    }

    day = 1 + ((dow - first_wday + 7) % 7) + (week - 1) * 7;
    if (day > dim) {
        day -= 7;
    }
    return day;
}

int64_t epoch_calendar_from_ymdhms(int year,
                                   int month,
                                   int day,
                                   int hour,
                                   int min,
                                   int sec,
                                   int16_t offset_min)
{
    int64_t days = 0;
    int y;

    for (y = 1970; y < year; y++) {
        days += epoch_calendar_is_leap_year(y) ? 366 : 365;
    }

    for (int m = 1; m < month; m++) {
        days += epoch_calendar_days_in_month(year, m);
    }

    days += day - 1;
    return days * 86400LL + hour * 3600LL + min * 60LL + sec -
           (int64_t)offset_min * 60LL;
}

void epoch_calendar_from_epoch(int64_t epoch,
                               uint16_t *year_out,
                               uint8_t *month_out,
                               uint8_t *day_out,
                               uint8_t *hour_out,
                               uint8_t *min_out,
                               uint8_t *sec_out,
                               uint8_t *wday_sun0_out)
{
    int64_t days;
    int64_t rem;
    int year = 1970;
    int month = 1;
    int wday;

    if (epoch < 0) {
        days = epoch / 86400LL;
        rem = epoch % 86400LL;

        if (rem < 0) {
            rem += 86400LL;
            days -= 1;
        }

        year = 1970;

        while (days < 0) {
            year--;
            days += epoch_calendar_is_leap_year(year) ? 366 : 365;
        }

        while (true) {
            int year_days = epoch_calendar_is_leap_year(year) ? 366 : 365;

            if (days < year_days) {
                break;
            }
            days -= year_days;
            year++;
        }

        month = 1;
        while (true) {
            int month_days = epoch_calendar_days_in_month(year, month);

            if (days < month_days) {
                break;
            }
            days -= month_days;
            month++;
        }

        if (year_out != NULL) {
            *year_out = (uint16_t)year;
        }
        if (month_out != NULL) {
            *month_out = (uint8_t)month;
        }
        if (day_out != NULL) {
            *day_out = (uint8_t)(days + 1);
        }
        if (hour_out != NULL) {
            *hour_out = (uint8_t)(rem / 3600LL);
        }
        if (min_out != NULL) {
            *min_out = (uint8_t)((rem % 3600LL) / 60LL);
        }
        if (sec_out != NULL) {
            *sec_out = (uint8_t)(rem % 60LL);
        }
        if (wday_sun0_out != NULL) {
            wday = (int)((epoch / 86400LL + 4LL) % 7LL);

            if (wday < 0) {
                wday += 7;
            }
            *wday_sun0_out = (uint8_t)wday;
        }
        return;
    }

    days = epoch / 86400LL;
    rem = epoch % 86400LL;
    if (rem < 0) {
        rem += 86400LL;
        days -= 1;
    }

    wday = (int)((days + 4LL) % 7LL);
    if (wday < 0) {
        wday += 7;
    }

    while (true) {
        int year_days = epoch_calendar_is_leap_year(year) ? 366 : 365;

        if (days < year_days) {
            break;
        }

        days -= year_days;
        year++;
    }

    while (true) {
        int month_days = epoch_calendar_days_in_month(year, month);

        if (days < month_days) {
            break;
        }

        days -= month_days;
        month++;
    }

    if (year_out != NULL) {
        *year_out = (uint16_t)year;
    }
    if (month_out != NULL) {
        *month_out = (uint8_t)month;
    }
    if (day_out != NULL) {
        *day_out = (uint8_t)(days + 1);
    }
    if (hour_out != NULL) {
        *hour_out = (uint8_t)(rem / 3600LL);
    }
    if (min_out != NULL) {
        *min_out = (uint8_t)((rem % 3600LL) / 60LL);
    }
    if (sec_out != NULL) {
        *sec_out = (uint8_t)(rem % 60LL);
    }
    if (wday_sun0_out != NULL) {
        *wday_sun0_out = (uint8_t)wday;
    }
}
