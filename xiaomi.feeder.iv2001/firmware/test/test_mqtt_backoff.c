/* Tests: spec/30-processes/mqtt-protocol.md (Reconnect strategy) */

#include "unity.h"
#include "mqtt_backoff.h"

void test_backoff_initial_delay_is_one_second(void)
{
    mqtt_backoff_t backoff;

    mqtt_backoff_init(&backoff);
    TEST_ASSERT_EQUAL_UINT32(MQTT_BACKOFF_INITIAL_MS, mqtt_backoff_current_ms(&backoff));
}

void test_backoff_doubles_on_failure(void)
{
    mqtt_backoff_t backoff;

    mqtt_backoff_init(&backoff);
    mqtt_backoff_on_failure(&backoff);
    TEST_ASSERT_EQUAL_UINT32(2000u, mqtt_backoff_current_ms(&backoff));
    mqtt_backoff_on_failure(&backoff);
    TEST_ASSERT_EQUAL_UINT32(4000u, mqtt_backoff_current_ms(&backoff));
}

void test_backoff_caps_at_sixty_seconds(void)
{
    mqtt_backoff_t backoff;
    int i;

    mqtt_backoff_init(&backoff);
    for (i = 0; i < 10; i++) {
        mqtt_backoff_on_failure(&backoff);
    }
    TEST_ASSERT_EQUAL_UINT32(MQTT_BACKOFF_MAX_MS, mqtt_backoff_current_ms(&backoff));
}

void test_backoff_resets_on_success(void)
{
    mqtt_backoff_t backoff;

    mqtt_backoff_init(&backoff);
    mqtt_backoff_on_failure(&backoff);
    mqtt_backoff_on_failure(&backoff);
    mqtt_backoff_on_success(&backoff);
    TEST_ASSERT_EQUAL_UINT32(MQTT_BACKOFF_INITIAL_MS, mqtt_backoff_current_ms(&backoff));
}

