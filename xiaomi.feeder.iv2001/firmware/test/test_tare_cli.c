/* Tests: spec/30-processes/uart-console.md § tare */

#include <string.h>

#include "unity.h"

#include "app_log.h"
#include "app_tare_cli.h"
#include "auto_tare.h"
#include "cli_test_assert.h"
#include "feeder_runtime.h"

static void tare_cli_test_reset(void)
{
    feeder_runtime_test_reset();
    auto_tare_test_reset();
    cli_test_reset();
}

void test_tare_cli_show_pending_unset_stable(void)
{
    tare_cli_test_reset();
    TEST_ASSERT_EQUAL_UINT8(0, tare_cli_run_show());

    TEST_ASSERT_NOT_NULL(strstr(app_log_test_line_from_end(2), "tare stable: (unset)"));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_line_from_end(1), "tare drift: 0.0 g"));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_line_from_end(0), "tare pending: yes"));
}

void test_tare_cli_show_anchored_state(void)
{
    tare_cli_test_reset();
    auto_tare_anchor(420);
    auto_tare_idle_sample(420, true);

    TEST_ASSERT_EQUAL_UINT8(0, tare_cli_run_show());

    TEST_ASSERT_NOT_NULL(strstr(app_log_test_line_from_end(2), "tare stable: 42.0 g"));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_line_from_end(1), "tare drift: 0.0 g"));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_line_from_end(0), "tare pending: no"));
}

void test_tare_cli_show_reports_drift_offset(void)
{
    tare_cli_test_reset();
    auto_tare_anchor(120);
    auto_tare_idle_sample(120, true);
    auto_tare_idle_sample(119, true);

    TEST_ASSERT_EQUAL_UINT8(0, tare_cli_run_show());

    TEST_ASSERT_NOT_NULL(strstr(app_log_test_line_from_end(2), "tare stable: 12.0 g"));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_line_from_end(1), "tare drift: 0.1 g"));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_line_from_end(0), "tare pending: no"));
}
