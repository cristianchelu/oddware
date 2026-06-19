/* Tests: spec/30-processes/time-sync.md */

#include "unity.h"

#include "epoch_calendar.h"

void test_epoch_calendar_from_ymdhms_unix_epoch(void)
{
    TEST_ASSERT_EQUAL_INT64(0,
                            epoch_calendar_from_ymdhms(1970, 1, 1, 0, 0, 0, 0));
}

void test_epoch_calendar_from_ymdhms_applies_offset(void)
{
    TEST_ASSERT_EQUAL_INT64(28800LL,
                            epoch_calendar_from_ymdhms(1970, 1, 1, 8, 0, 0, 0));
    TEST_ASSERT_EQUAL_INT64(0,
                            epoch_calendar_from_ymdhms(1970, 1, 1, 8, 0, 0, 480));
}

void test_epoch_calendar_from_epoch_unix_epoch(void)
{
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
    uint8_t hour = 0;
    uint8_t min = 0;
    uint8_t sec = 0;
    uint8_t wday = 0;

    epoch_calendar_from_epoch(0, &year, &month, &day, &hour, &min, &sec, &wday);
    TEST_ASSERT_EQUAL_UINT16(1970, year);
    TEST_ASSERT_EQUAL_UINT8(1, month);
    TEST_ASSERT_EQUAL_UINT8(1, day);
    TEST_ASSERT_EQUAL_UINT8(0, hour);
    TEST_ASSERT_EQUAL_UINT8(0, min);
    TEST_ASSERT_EQUAL_UINT8(0, sec);
    TEST_ASSERT_EQUAL_UINT8(4, wday);
}

void test_epoch_calendar_round_trip_positive(void)
{
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
    uint8_t hour = 0;
    uint8_t min = 0;
    uint8_t sec = 0;
    int64_t epoch = epoch_calendar_from_ymdhms(2024, 7, 1, 12, 30, 45, 0);

    epoch_calendar_from_epoch(epoch, &year, &month, &day, &hour, &min, &sec, NULL);
    TEST_ASSERT_EQUAL_UINT16(2024, year);
    TEST_ASSERT_EQUAL_UINT8(7, month);
    TEST_ASSERT_EQUAL_UINT8(1, day);
    TEST_ASSERT_EQUAL_UINT8(12, hour);
    TEST_ASSERT_EQUAL_UINT8(30, min);
    TEST_ASSERT_EQUAL_UINT8(45, sec);
}

void test_epoch_calendar_round_trip_negative(void)
{
    uint16_t year = 0;
    uint8_t month = 0;
    uint8_t day = 0;
    uint8_t hour = 0;
    uint8_t min = 0;
    uint8_t sec = 0;
    int64_t epoch = -3600LL;

    epoch_calendar_from_epoch(epoch, &year, &month, &day, &hour, &min, &sec, NULL);
    TEST_ASSERT_EQUAL_UINT16(1969, year);
    TEST_ASSERT_EQUAL_UINT8(12, month);
    TEST_ASSERT_EQUAL_UINT8(31, day);
    TEST_ASSERT_EQUAL_UINT8(23, hour);
    TEST_ASSERT_EQUAL_UINT8(0, min);
    TEST_ASSERT_EQUAL_UINT8(0, sec);
}

void test_epoch_calendar_nth_weekday_second_sunday(void)
{
    int day = epoch_calendar_nth_weekday(2024, 3, 2, 0);

    TEST_ASSERT_EQUAL_INT(9, day);
    TEST_ASSERT_EQUAL_INT(0, epoch_calendar_weekday_sun0(2024, 3, day));
}

void test_epoch_calendar_is_leap_year(void)
{
    TEST_ASSERT_TRUE(epoch_calendar_is_leap_year(2024));
    TEST_ASSERT_FALSE(epoch_calendar_is_leap_year(2023));
    TEST_ASSERT_TRUE(epoch_calendar_is_leap_year(2000));
    TEST_ASSERT_FALSE(epoch_calendar_is_leap_year(1900));
}
