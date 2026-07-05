/* Tests: spec/30-processes/web-ui.md */

#include <string.h>

#include "unity.h"

#include "fake_config_port.h"
#include "feed_config.h"
#include "schedule.h"
#include "web_api.h"

void test_web_api_classify_schedule_state_get(void)
{
    TEST_ASSERT_EQUAL(WEB_API_GET_SCHEDULE_STATE,
                      web_api_classify("GET", "/api/schedule/state"));
}

void test_web_api_classify_schedule_set_post(void)
{
    TEST_ASSERT_EQUAL(WEB_API_POST_SCHEDULE_SET,
                      web_api_classify("POST", "/api/schedule/set"));
}

void test_web_api_classify_unknown_path(void)
{
    TEST_ASSERT_EQUAL(WEB_API_ROUTE_UNKNOWN, web_api_classify("GET", "/api/nope"));
}

void test_web_api_handle_get_schedule_state(void)
{
    char buf[512];
    int written;

    fake_config_port_reset();
    schedule_test_reset();
    schedule_init();

    written = web_api_handle_get(WEB_API_GET_SCHEDULE_STATE, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"schedule\""));
}

void test_web_api_handle_post_schedule_set(void)
{
    char resp[128];
    const char *payload =
        "{\"hour\":7,\"min\":15,\"repeat_days\":[0,1,2,3,4,5,6],\"g\":40,\"enabled\":true}";
    int written;

    fake_config_port_reset();
    schedule_test_reset();
    schedule_init();

    written = web_api_handle_post(WEB_API_POST_SCHEDULE_SET,
                                  payload,
                                  strlen(payload),
                                  resp,
                                  sizeof(resp));
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(resp, "\"ok\":true"));
    TEST_ASSERT_EQUAL(1, schedule_slot_count());
}

void test_web_api_handle_get_feed_mode(void)
{
    char buf[32];
    int written;

    fake_config_port_reset();
    feed_config_mode_set(DISPENSE_MODE_COMPENSATED);

    written = web_api_handle_get(WEB_API_GET_FEED_MODE, buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_EQUAL_STRING("compensated", buf);
}
