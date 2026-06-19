/* Tests: spec/30-processes/scheduler-engine.md § Local timezone rule */

#include <string.h>

#include "unity.h"

#include "fake_config_port.h"
#include "tz_rule.h"

void test_tz_rule_parse_utc_shorthand(void)
{
    tz_rule_t rule;

    TEST_ASSERT_TRUE(tz_rule_parse_wire("0", &rule));
    TEST_ASSERT_EQUAL_INT16(0, rule.std_offset_min);
    TEST_ASSERT_EQUAL_INT16(0, rule.dst_offset_min);
    TEST_ASSERT_FALSE(tz_rule_dst_enabled(&rule));
}

void test_tz_rule_parse_fixed_offset(void)
{
    tz_rule_t rule;

    TEST_ASSERT_TRUE(tz_rule_parse_wire("480", &rule));
    TEST_ASSERT_EQUAL_INT16(480, rule.std_offset_min);
    TEST_ASSERT_EQUAL_INT16(480, rule.dst_offset_min);
}

void test_tz_rule_parse_us_eastern(void)
{
    tz_rule_t rule;

    TEST_ASSERT_TRUE(tz_rule_parse_wire("-300/-240/3.2.0.2/11.1.0.2", &rule));
    TEST_ASSERT_EQUAL_INT16(-300, rule.std_offset_min);
    TEST_ASSERT_EQUAL_INT16(-240, rule.dst_offset_min);
    TEST_ASSERT_EQUAL_UINT8(3, rule.start_m);
    TEST_ASSERT_EQUAL_UINT8(2, rule.start_w);
    TEST_ASSERT_EQUAL_UINT8(0, rule.start_d);
    TEST_ASSERT_EQUAL_UINT8(2, rule.start_h);
}

void test_tz_rule_wire_round_trip(void)
{
    tz_rule_t rule;
    tz_rule_t parsed;
    char wire[TZ_RULE_WIRE_MAX];

    TEST_ASSERT_TRUE(tz_rule_parse_wire("60/120/3.5.0.2/10.5.0.3", &rule));
    TEST_ASSERT_TRUE(tz_rule_format_wire(&rule, wire, sizeof(wire)));
    TEST_ASSERT_TRUE(tz_rule_parse_wire(wire, &parsed));
    TEST_ASSERT_EQUAL_INT16(rule.std_offset_min, parsed.std_offset_min);
    TEST_ASSERT_EQUAL_INT16(rule.dst_offset_min, parsed.dst_offset_min);
    TEST_ASSERT_EQUAL_UINT8(rule.start_m, parsed.start_m);
    TEST_ASSERT_EQUAL_UINT8(rule.end_h, parsed.end_h);
}

void test_tz_rule_pack_round_trip(void)
{
    tz_rule_t rule;
    tz_rule_t unpacked;
    uint8_t packed[TZ_RULE_PACKED_SIZE];

    TEST_ASSERT_TRUE(tz_rule_parse_wire("-300/-240/3.2.0.2/11.1.0.2", &rule));
    TEST_ASSERT_TRUE(tz_rule_pack(&rule, packed, sizeof(packed)));
    TEST_ASSERT_TRUE(tz_rule_unpack(packed, sizeof(packed), &unpacked));
    TEST_ASSERT_EQUAL_INT16(-300, unpacked.std_offset_min);
    TEST_ASSERT_EQUAL_INT16(-240, unpacked.dst_offset_min);
}

void test_tz_rule_us_eastern_dst_offsets(void)
{
    tz_rule_t rule;
    int64_t winter = 1705320000LL; /* 2024-01-15 12:00:00 UTC */
    int64_t summer = 1719835200LL; /* 2024-07-01 12:00:00 UTC */

    TEST_ASSERT_TRUE(tz_rule_parse_wire("-300/-240/3.2.0.2/11.1.0.2", &rule));
    TEST_ASSERT_EQUAL_INT16(-300, tz_rule_effective_offset_min(&rule, winter));
    TEST_ASSERT_EQUAL_INT16(-240, tz_rule_effective_offset_min(&rule, summer));
}

void test_tz_rule_rejects_invalid_wire(void)
{
    tz_rule_t rule;

    TEST_ASSERT_FALSE(tz_rule_parse_wire("", &rule));
    TEST_ASSERT_FALSE(tz_rule_parse_wire("abc", &rule));
    TEST_ASSERT_FALSE(tz_rule_parse_wire("9999", &rule));
}

void test_tz_rule_save_and_load_blob(void)
{
    tz_rule_t rule;
    tz_rule_t loaded;
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    TEST_ASSERT_TRUE(tz_rule_parse_wire("480", &rule));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_save(cfg, &rule));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_load(cfg, &loaded));
    TEST_ASSERT_EQUAL_INT16(480, loaded.std_offset_min);
}
