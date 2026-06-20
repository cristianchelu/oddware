/* Tests: spec/30-processes/config-store.md, scheduler-engine.md */

#include <string.h>

#include "unity.h"

#include "fake_config_port.h"
#include "fake_mqtt_port.h"
#include "fake_time.h"
#include "fake_time_port.h"
#include "mqtt_config.h"
#include "mqtt_outbox.h"
#include "time_config.h"
#include "time_sync.h"
#include "tz_rule.h"

#define TEST_DEVICE_ID "aabbcc"
#define BUCHAREST_POSIX "EET-2EEST,M3.5.0/3,M10.5.0/4"

static void drain_config_outbox(void)
{
    const mqtt_port_t *mqtt = fake_mqtt_port_get();

    while (mqtt_outbox_pending() > 0) {
        if (!mqtt_outbox_drain_one(mqtt)) {
            fake_time_advance_ms(101u);
            (void)mqtt_outbox_drain_one(mqtt);
        }
    }
}

static void setup_time_config(void)
{
    fake_config_port_reset();
    fake_mqtt_port_reset();
    fake_time_port_reset();
    mqtt_outbox_reset();
    mqtt_config_test_reset();
    time_sync_test_reset();
    tz_rule_test_reset();
    fake_mqtt_port_get()->connect(NULL);
    mqtt_config_set_device_id(TEST_DEVICE_ID);
}

void test_time_config_apply_posix_saves_nvdm(void)
{
    char loaded[TZ_RULE_POSIX_MAX];
    const config_port_t *cfg = fake_config_port_get();
    time_config_patch_t patch = {
        .tz_rule_posix = BUCHAREST_POSIX,
        .tz_label = NULL,
    };

    fake_config_port_reset();
    tz_rule_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK, time_config_apply(cfg, &patch));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_load_posix(cfg, loaded, sizeof(loaded)));
    TEST_ASSERT_EQUAL_STRING(BUCHAREST_POSIX, loaded);
}

void test_time_config_apply_invalid_posix_rejects(void)
{
    char loaded[TZ_RULE_POSIX_MAX];
    const config_port_t *cfg = fake_config_port_get();
    time_config_patch_t seed = {
        .tz_rule_posix = BUCHAREST_POSIX,
        .tz_label = NULL,
    };
    time_config_patch_t bad = {
        .tz_rule_posix = "480",
        .tz_label = NULL,
    };

    fake_config_port_reset();
    tz_rule_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK, time_config_apply(cfg, &seed));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, time_config_apply(cfg, &bad));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_load_posix(cfg, loaded, sizeof(loaded)));
    TEST_ASSERT_EQUAL_STRING(BUCHAREST_POSIX, loaded);
}

void test_time_config_apply_label_round_trip(void)
{
    char label[TZ_RULE_LABEL_MAX];
    const config_port_t *cfg = fake_config_port_get();
    time_config_patch_t patch = {
        .tz_rule_posix = NULL,
        .tz_label = "Europe/Bucharest",
    };

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, time_config_apply(cfg, &patch));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_label_load(cfg, label, sizeof(label)));
    TEST_ASSERT_EQUAL_STRING("Europe/Bucharest", label);
}

void test_time_config_apply_overlong_label_rejects(void)
{
    time_config_patch_t patch = {
        .tz_rule_posix = NULL,
        .tz_label = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMN",
    };

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      time_config_apply(fake_config_port_get(), &patch));
}

void test_time_config_apply_empty_patch_rejects(void)
{
    time_config_patch_t patch = {
        .tz_rule_posix = NULL,
        .tz_label = NULL,
    };

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      time_config_apply(fake_config_port_get(), &patch));
}

void test_time_config_apply_posix_publishes_snapshot(void)
{
    const fake_mqtt_port_state_t *mqtt;
    time_config_patch_t patch = {
        .tz_rule_posix = BUCHAREST_POSIX,
        .tz_label = NULL,
    };

    setup_time_config();
    TEST_ASSERT_EQUAL(PORT_OK, time_config_apply(fake_config_port_get(), &patch));

    drain_config_outbox();
    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_EQUAL_UINT(1, mqtt->publish_calls);
    TEST_ASSERT_NOT_NULL(strstr(mqtt->last_publish_payload, BUCHAREST_POSIX));
}

void test_time_config_apply_invalid_label_does_not_change_rule(void)
{
    char loaded[TZ_RULE_POSIX_MAX];
    const config_port_t *cfg = fake_config_port_get();
    time_config_patch_t seed = {
        .tz_rule_posix = BUCHAREST_POSIX,
        .tz_label = NULL,
    };
    time_config_patch_t bad = {
        .tz_rule_posix = "UTC0",
        .tz_label = "abcdefghijklmnopqrstuvwxyz0123456789ABCDEFGHIJKLMN",
    };

    fake_config_port_reset();
    tz_rule_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK, time_config_apply(cfg, &seed));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, time_config_apply(cfg, &bad));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_load_posix(cfg, loaded, sizeof(loaded)));
    TEST_ASSERT_EQUAL_STRING(BUCHAREST_POSIX, loaded);
}
