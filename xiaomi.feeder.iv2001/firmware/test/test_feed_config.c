/* Tests: spec/30-processes/config-store.md (feed/child_lock, feed/overfill) */

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

void test_feed_config_mode_defaults_open_loop_when_missing(void)
{
    fake_config_port_reset();
    TEST_ASSERT_EQUAL(DISPENSE_MODE_OPEN_LOOP, feed_config_mode_get());
}

void test_feed_config_mode_set_and_load(void)
{
    const config_port_t *cfg = fake_config_port_get();
    char buf[16];

    fake_config_port_reset();
    TEST_ASSERT_TRUE(feed_config_mode_set(DISPENSE_MODE_COMPENSATED));
    TEST_ASSERT_EQUAL(DISPENSE_MODE_COMPENSATED, feed_config_mode_get());

    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg->read(CONFIG_GROUP_FEED, CONFIG_KEY_FEED_MODE, buf, sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("compensated", buf);

    TEST_ASSERT_TRUE(feed_config_mode_set(DISPENSE_MODE_OPEN_LOOP));
    TEST_ASSERT_EQUAL(DISPENSE_MODE_OPEN_LOOP, feed_config_mode_get());
}

void test_feed_config_mode_rejects_invalid_enum(void)
{
    fake_config_port_reset();
    TEST_ASSERT_FALSE(feed_config_mode_set((dispense_mode_t)99));
}

void test_feed_config_overfill_defaults_when_missing(void)
{
    fake_config_port_reset();
    TEST_ASSERT_FALSE(feed_config_overfill_enabled_get());
    TEST_ASSERT_EQUAL_UINT8(FEED_OVERFILL_THRESHOLD_G_DEFAULT,
                           feed_config_overfill_threshold_g_get());
}

void test_feed_config_overfill_set_and_load(void)
{
    const config_port_t *cfg = fake_config_port_get();
    char buf[8];

    fake_config_port_reset();
    TEST_ASSERT_TRUE(feed_config_overfill_enabled_set(true));
    TEST_ASSERT_TRUE(feed_config_overfill_threshold_g_set(40u));
    TEST_ASSERT_TRUE(feed_config_overfill_enabled_get());
    TEST_ASSERT_EQUAL_UINT8(40u, feed_config_overfill_threshold_g_get());

    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg->read(CONFIG_GROUP_FEED,
                                CONFIG_KEY_OVERFILL_ENABLED,
                                buf,
                                sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("1", buf);

    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg->read(CONFIG_GROUP_FEED,
                                CONFIG_KEY_OVERFILL_THRESHOLD_G,
                                buf,
                                sizeof(buf)));
    TEST_ASSERT_EQUAL_STRING("40", buf);
}

void test_feed_config_overfill_threshold_clamps_on_load(void)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg->write(CONFIG_GROUP_FEED,
                                 CONFIG_KEY_OVERFILL_THRESHOLD_G,
                                 "29"));
    TEST_ASSERT_EQUAL_UINT8(30u, feed_config_overfill_threshold_g_get());

    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg->write(CONFIG_GROUP_FEED,
                                 CONFIG_KEY_OVERFILL_THRESHOLD_G,
                                 "101"));
    TEST_ASSERT_EQUAL_UINT8(100u, feed_config_overfill_threshold_g_get());
}

void test_feed_config_overfill_threshold_rejects_out_of_range_set(void)
{
    fake_config_port_reset();
    TEST_ASSERT_FALSE(feed_config_overfill_threshold_g_set(29u));
    TEST_ASSERT_FALSE(feed_config_overfill_threshold_g_set(101u));
    TEST_ASSERT_TRUE(feed_config_overfill_threshold_g_set(30u));
    TEST_ASSERT_TRUE(feed_config_overfill_threshold_g_set(100u));
}

void test_feed_config_overfill_enabled_rejects_invalid_bool_on_load(void)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg->write(CONFIG_GROUP_FEED,
                                 CONFIG_KEY_OVERFILL_ENABLED,
                                 "2"));
    TEST_ASSERT_FALSE(feed_config_overfill_enabled_get());

    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg->write(CONFIG_GROUP_FEED,
                                 CONFIG_KEY_OVERFILL_ENABLED,
                                 "maybe"));
    TEST_ASSERT_FALSE(feed_config_overfill_enabled_get());
}
