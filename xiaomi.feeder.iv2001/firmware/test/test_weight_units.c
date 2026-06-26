/* Tests: spec/30-processes/weighing.md § Data model */

#include "unity.h"

#include "weight_units.h"

void test_weight_dg_to_g_round(void)
{
    TEST_ASSERT_EQUAL_INT32(42, WEIGHT_DG_TO_G_ROUND(423));
    TEST_ASSERT_EQUAL_INT32(43, WEIGHT_DG_TO_G_ROUND(425));
    TEST_ASSERT_EQUAL_INT32(0, WEIGHT_DG_TO_G_ROUND(-3));
}

void test_weight_dg_to_display_g(void)
{
    TEST_ASSERT_EQUAL_UINT16(42, weight_dg_to_display_g(423));
    TEST_ASSERT_EQUAL_UINT16(43, weight_dg_to_display_g(425));
    TEST_ASSERT_EQUAL_UINT16(0, weight_dg_to_display_g(-3));
    TEST_ASSERT_EQUAL_UINT16(999, weight_dg_to_display_g(10000));
}

void test_weight_format_mqtt_g(void)
{
    char buf[16];

    TEST_ASSERT_EQUAL(4, weight_format_mqtt_g(423, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("42.3", buf);

    TEST_ASSERT_EQUAL(3, weight_format_mqtt_g(0, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("0.0", buf);

    TEST_ASSERT_EQUAL(3, weight_format_mqtt_g(-5, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("0.0", buf);
}

void test_weight_format_cli_g(void)
{
    char buf[16];

    TEST_ASSERT_EQUAL(4, weight_format_cli_g(423, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("42.3", buf);

    TEST_ASSERT_EQUAL(4, weight_format_cli_g(-12, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("-1.2", buf);
}
