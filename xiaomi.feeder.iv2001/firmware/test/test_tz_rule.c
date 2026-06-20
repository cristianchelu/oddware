/* Tests: spec/30-processes/scheduler-engine.md § Local timezone rule */

#include <string.h>

#include "unity.h"

#include "fake_config_port.h"
#include "tz_rule.h"

#include "config_keys.h"

void test_tz_rule_parse_utc_fixed(void)
{
    tz_rule_t rule;

    TEST_ASSERT_TRUE(tz_rule_parse_posix("UTC0", &rule));
    TEST_ASSERT_EQUAL_INT16(0, rule.std_offset_min);
    TEST_ASSERT_EQUAL_INT16(0, rule.dst_offset_min);
    TEST_ASSERT_FALSE(tz_rule_dst_enabled(&rule));
}

void test_tz_rule_parse_fixed_offset_east(void)
{
    tz_rule_t rule;

    TEST_ASSERT_TRUE(tz_rule_parse_posix("CST-8", &rule));
    TEST_ASSERT_EQUAL_INT16(480, rule.std_offset_min);
    TEST_ASSERT_EQUAL_INT16(480, rule.dst_offset_min);
}

void test_tz_rule_parse_us_eastern(void)
{
    tz_rule_t rule;

    TEST_ASSERT_TRUE(tz_rule_parse_posix("EST5EDT,M3.2.0,M11.1.0", &rule));
    TEST_ASSERT_EQUAL_INT16(-300, rule.std_offset_min);
    TEST_ASSERT_EQUAL_INT16(-240, rule.dst_offset_min);
    TEST_ASSERT_EQUAL_UINT8(3, rule.start_m);
    TEST_ASSERT_EQUAL_UINT8(2, rule.start_w);
    TEST_ASSERT_EQUAL_UINT8(0, rule.start_d);
    TEST_ASSERT_EQUAL_UINT8(2, rule.start_h);
}

void test_tz_rule_parse_bucharest(void)
{
    tz_rule_t rule;

    TEST_ASSERT_TRUE(tz_rule_parse_posix("EET-2EEST,M3.5.0/3,M10.5.0/4", &rule));
    TEST_ASSERT_EQUAL_INT16(120, rule.std_offset_min);
    TEST_ASSERT_EQUAL_INT16(180, rule.dst_offset_min);
    TEST_ASSERT_EQUAL_UINT8(3, rule.start_h);
    TEST_ASSERT_EQUAL_UINT8(4, rule.end_h);
}

void test_tz_rule_parse_fractional_offset(void)
{
    tz_rule_t rule;

    TEST_ASSERT_TRUE(tz_rule_parse_posix("IST-5:30", &rule));
    TEST_ASSERT_EQUAL_INT16(330, rule.std_offset_min);
}

void test_tz_rule_us_eastern_dst_offsets(void)
{
    tz_rule_t rule;
    int64_t winter = 1705320000LL;
    int64_t summer = 1719835200LL;

    TEST_ASSERT_TRUE(tz_rule_parse_posix("EST5EDT,M3.2.0,M11.1.0", &rule));
    TEST_ASSERT_EQUAL_INT16(-300, tz_rule_effective_offset_min(&rule, winter));
    TEST_ASSERT_EQUAL_INT16(-240, tz_rule_effective_offset_min(&rule, summer));
}

void test_tz_rule_rejects_invalid_posix(void)
{
    tz_rule_t rule;

    TEST_ASSERT_FALSE(tz_rule_parse_posix("", &rule));
    TEST_ASSERT_FALSE(tz_rule_parse_posix("480", &rule));
    TEST_ASSERT_FALSE(tz_rule_parse_posix("60/120/3.5.0.2/10.5.0.3", &rule));
    TEST_ASSERT_FALSE(tz_rule_parse_posix("EEST", &rule));
    TEST_ASSERT_FALSE(tz_rule_parse_posix("EET-2EEST,J91/0,M9.2.4", &rule));
}

void test_tz_rule_save_and_load_posix_string(void)
{
    char loaded[TZ_RULE_POSIX_MAX];
    const config_port_t *cfg = fake_config_port_get();
    const char *posix = "EET-2EEST,M3.5.0/3,M10.5.0/4";

    fake_config_port_reset();
    tz_rule_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_save_posix(cfg, posix));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_load_posix(cfg, loaded, sizeof(loaded)));
    TEST_ASSERT_EQUAL_STRING(posix, loaded);
    TEST_ASSERT_EQUAL_INT16(120, tz_rule_get()->std_offset_min);
}

void test_tz_rule_init_loads_cache(void)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    tz_rule_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK,
                      tz_rule_save_posix(cfg, "EST5EDT,M3.2.0,M11.1.0"));
    tz_rule_init();
    TEST_ASSERT_EQUAL_INT16(-300, tz_rule_get()->std_offset_min);
}

void test_tz_rule_init_unparseable_nvdm_defaults_utc(void)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    tz_rule_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg->write(CONFIG_GROUP_TIME, CONFIG_KEY_TZ_RULE, "not-valid!!!"));
    tz_rule_init();
    TEST_ASSERT_EQUAL_INT16(0, tz_rule_get()->std_offset_min);
    TEST_ASSERT_EQUAL_INT16(0, tz_rule_get()->dst_offset_min);
    TEST_ASSERT_EQUAL_INT16(0,
                            tz_rule_effective_offset_min(tz_rule_get(), 1705320000LL));
}
