/* Tests: spec/30-processes/config-store.md (feed/child_lock) */

#include <string.h>

#include "unity.h"

#include "config_keys.h"
#include "fake_config_port.h"
#include "feed_config.h"

void test_feed_config_child_lock_defaults_false_when_missing(void)
{
    fake_config_port_reset();
    TEST_ASSERT_FALSE(feed_config_child_lock_is_active());
}

void test_feed_config_child_lock_set_and_load(void)
{
    const config_port_t *cfg = fake_config_port_get();
    char buf[8];

    fake_config_port_reset();
    TEST_ASSERT_TRUE(feed_config_child_lock_set(true));
    TEST_ASSERT_TRUE(feed_config_child_lock_is_active());

    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg->read(CONFIG_GROUP_FEED, CONFIG_KEY_CHILD_LOCK, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("1", buf);

    TEST_ASSERT_TRUE(feed_config_child_lock_set(false));
    TEST_ASSERT_FALSE(feed_config_child_lock_is_active());
}

void test_feed_config_child_lock_toggle_round_trip(void)
{
    fake_config_port_reset();

    TEST_ASSERT_TRUE(feed_config_child_lock_toggle());
    TEST_ASSERT_TRUE(feed_config_child_lock_is_active());

    TEST_ASSERT_FALSE(feed_config_child_lock_toggle());
    TEST_ASSERT_FALSE(feed_config_child_lock_is_active());
}
