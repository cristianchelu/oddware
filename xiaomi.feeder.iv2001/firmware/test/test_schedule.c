/* Tests: spec/30-processes/scheduler-engine.md, mqtt-protocol.md § Schedule */

#include <string.h>

#include "unity.h"

#include "config_keys.h"
#include "fake_config_port.h"
#include "fake_time_port.h"
#include "fake_weight_port.h"
#include "app.h"
#include "feed_bowl.h"
#include "feed_config.h"
#include "schedule.h"
#include "schedule_nvdm.h"
#include "schedule_test_epochs.h"
#include "time_sync.h"
#include "tz_rule.h"
#include "weight_units.h"

static unsigned s_fire_hour;
static unsigned s_fire_min;
static unsigned s_fire_g;
static unsigned s_fire_calls;
static schedule_fire_result_t s_fire_result;

static schedule_fire_result_t test_schedule_fire(uint8_t hour, uint8_t min, uint8_t g)
{
    s_fire_hour = (unsigned)hour;
    s_fire_min = (unsigned)min;
    s_fire_g = (unsigned)g;
    s_fire_calls++;
    return s_fire_result;
}

static schedule_fire_result_t test_schedule_fire_with_overfill(uint8_t hour,
                                                               uint8_t min,
                                                               uint8_t g)
{
    (void)hour;
    (void)min;
    s_fire_g = (unsigned)g;
    s_fire_calls++;
    return feed_schedule_fire(g, 1000u);
}

static void setup_schedule_synced(int64_t epoch)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    fake_time_port_reset();
    time_sync_test_reset();
    tz_rule_test_reset();
    schedule_test_reset();

    s_fire_hour = 0u;
    s_fire_min = 0u;
    s_fire_g = 0u;
    s_fire_calls = 0u;
    s_fire_result = SCHEDULE_FIRE_OK;

    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_save_posix(cfg, "UTC0"));
    tz_rule_init();

    fake_time_port_set_epoch(epoch);
    time_sync_init();
    time_sync_on_wifi_ready();
    fake_time_port_queue_sync_result(true, epoch);
    time_sync_poll(1000u);
    TEST_ASSERT_TRUE(time_sync_is_valid());

    schedule_init();
    schedule_set_fire_fn(test_schedule_fire);
}

void test_schedule_set_slot_persists_and_sorts(void)
{
    schedule_slot_config_t morning = {
        .hour = 8,
        .min = 0,
        .days = 127,
        .g = 30,
        .enabled = true,
    };
    schedule_slot_config_t evening = {
        .hour = 18,
        .min = 30,
        .days = 127,
        .g = 20,
        .enabled = true,
    };
    schedule_slot_config_t cfg;
    schedule_slot_runtime_t rt;

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    TEST_ASSERT_TRUE(schedule_set_slot(&evening));
    TEST_ASSERT_TRUE(schedule_set_slot(&morning));
    TEST_ASSERT_EQUAL_size_t(2, schedule_slot_count());

    TEST_ASSERT_TRUE(schedule_get_slot(0, &cfg, &rt));
    TEST_ASSERT_EQUAL_UINT8(8, cfg.hour);
    TEST_ASSERT_EQUAL_UINT8(0, cfg.min);

    TEST_ASSERT_TRUE(schedule_get_slot(1, &cfg, NULL));
    TEST_ASSERT_EQUAL_UINT8(18, cfg.hour);
    TEST_ASSERT_EQUAL_UINT8(30, cfg.min);

    {
        const config_port_t *cfg_port = fake_config_port_get();
        schedule_nvdm_config_t stored;
        size_t blob_len = 0;

        TEST_ASSERT_EQUAL(PORT_OK,
                          cfg_port->read_blob(CONFIG_GROUP_SCHEDULE,
                                              CONFIG_KEY_SCHEDULE_SLOTS,
                                              &stored,
                                              sizeof(stored),
                                              &blob_len));
        TEST_ASSERT_EQUAL_size_t(sizeof(stored), blob_len);
        TEST_ASSERT_EQUAL_UINT32(SCHEDULE_NVDM_CONFIG_MAGIC, stored.magic);
        TEST_ASSERT_EQUAL_UINT8(SCHEDULE_NVDM_CONFIG_VERSION, stored.version);
        TEST_ASSERT_EQUAL_UINT8(2, stored.count);
    }
}

void test_schedule_nvdm_32_slot_round_trip(void)
{
    schedule_slot_config_t slot = {
        .days = 127,
        .g = SCHEDULE_G_MAX,
        .enabled = true,
    };
    schedule_slot_config_t loaded;
    const config_port_t *cfg_port = fake_config_port_get();
    schedule_nvdm_config_t stored;
    size_t blob_len = 0;
    size_t i;

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    for (i = 0; i < SCHEDULE_MAX_SLOTS; i++) {
        slot.hour = 0;
        slot.min = (uint8_t)i;
        TEST_ASSERT_TRUE(schedule_set_slot(&slot));
    }

    TEST_ASSERT_EQUAL_size_t(SCHEDULE_MAX_SLOTS, schedule_slot_count());

    schedule_test_reset();
    schedule_init();
    schedule_set_fire_fn(test_schedule_fire);

    TEST_ASSERT_EQUAL_size_t(SCHEDULE_MAX_SLOTS, schedule_slot_count());

    for (i = 0; i < SCHEDULE_MAX_SLOTS; i++) {
        TEST_ASSERT_TRUE(schedule_get_slot(i, &loaded, NULL));
        TEST_ASSERT_EQUAL_UINT8(0, loaded.hour);
        TEST_ASSERT_EQUAL_UINT8((uint8_t)i, loaded.min);
        TEST_ASSERT_EQUAL_UINT8(127, loaded.days);
        TEST_ASSERT_EQUAL_UINT8(SCHEDULE_G_MAX, loaded.g);
        TEST_ASSERT_TRUE(loaded.enabled);
    }

    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg_port->read_blob(CONFIG_GROUP_SCHEDULE,
                                          CONFIG_KEY_SCHEDULE_SLOTS,
                                          &stored,
                                          sizeof(stored),
                                          &blob_len));
    TEST_ASSERT_EQUAL_size_t(sizeof(stored), blob_len);
    TEST_ASSERT_EQUAL_UINT8(SCHEDULE_MAX_SLOTS, stored.count);
    TEST_ASSERT_EQUAL_UINT8(SCHEDULE_G_MAX, stored.slots[0].g);
    TEST_ASSERT_EQUAL_UINT8(SCHEDULE_G_MAX, stored.slots[SCHEDULE_MAX_SLOTS - 1u].g);
}

void test_schedule_duplicate_keys_in_nvdm_heals(void)
{
    const config_port_t *cfg_port = fake_config_port_get();
    schedule_nvdm_config_t bad;
    schedule_nvdm_config_t stored;
    size_t blob_len = 0;

    memset(&bad, 0, sizeof(bad));
    bad.magic = SCHEDULE_NVDM_CONFIG_MAGIC;
    bad.version = SCHEDULE_NVDM_CONFIG_VERSION;
    bad.count = 2;
    bad.slots[0].hour = 8;
    bad.slots[0].min = 0;
    bad.slots[0].days = 127;
    bad.slots[0].g = 30;
    bad.slots[0].flags = SCHEDULE_NVDM_SLOT_FLAG_ENABLED;
    bad.slots[1] = bad.slots[0];

    fake_config_port_reset();
    time_sync_test_reset();
    tz_rule_test_reset();
    schedule_test_reset();

    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg_port->write_blob(CONFIG_GROUP_SCHEDULE,
                                           CONFIG_KEY_SCHEDULE_SLOTS,
                                           &bad,
                                           sizeof(bad)));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_save_posix(cfg_port, "UTC0"));
    tz_rule_init();
    schedule_init();

    TEST_ASSERT_EQUAL_size_t(0, schedule_slot_count());

    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg_port->read_blob(CONFIG_GROUP_SCHEDULE,
                                          CONFIG_KEY_SCHEDULE_SLOTS,
                                          &stored,
                                          sizeof(stored),
                                          &blob_len));
    TEST_ASSERT_EQUAL_size_t(sizeof(stored), blob_len);
    TEST_ASSERT_EQUAL_UINT8(0, stored.count);
}

void test_schedule_wrong_blob_size_heals(void)
{
    const config_port_t *cfg_port = fake_config_port_get();
    uint8_t short_blob[64];
    schedule_nvdm_config_t stored;
    size_t blob_len = 0;

    memset(short_blob, 0, sizeof(short_blob));

    fake_config_port_reset();
    time_sync_test_reset();
    tz_rule_test_reset();
    schedule_test_reset();

    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg_port->write_blob(CONFIG_GROUP_SCHEDULE,
                                           CONFIG_KEY_SCHEDULE_SLOTS,
                                           short_blob,
                                           sizeof(short_blob)));
    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_save_posix(cfg_port, "UTC0"));
    tz_rule_init();
    schedule_init();

    TEST_ASSERT_EQUAL_size_t(0, schedule_slot_count());

    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg_port->read_blob(CONFIG_GROUP_SCHEDULE,
                                          CONFIG_KEY_SCHEDULE_SLOTS,
                                          &stored,
                                          sizeof(stored),
                                          &blob_len));
    TEST_ASSERT_EQUAL_size_t(sizeof(stored), blob_len);
    TEST_ASSERT_EQUAL_UINT32(SCHEDULE_NVDM_CONFIG_MAGIC, stored.magic);
    TEST_ASSERT_EQUAL_UINT8(0, stored.count);
}

void test_schedule_corrupt_nvdm_resets_and_heals(void)
{
    const config_port_t *cfg_port = fake_config_port_get();
    uint8_t garbage[sizeof(schedule_nvdm_config_t)];
    schedule_nvdm_config_t stored;
    size_t blob_len = 0;

    memset(garbage, 0xff, sizeof(garbage));

    fake_config_port_reset();
    fake_time_port_reset();
    time_sync_test_reset();
    tz_rule_test_reset();
    schedule_test_reset();

    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg_port->write_blob(CONFIG_GROUP_SCHEDULE,
                                           CONFIG_KEY_SCHEDULE_SLOTS,
                                           garbage,
                                           sizeof(garbage)));

    TEST_ASSERT_EQUAL(PORT_OK, tz_rule_save_posix(cfg_port, "UTC0"));
    tz_rule_init();
    schedule_init();

    TEST_ASSERT_EQUAL_size_t(0, schedule_slot_count());

    TEST_ASSERT_EQUAL(PORT_OK,
                      cfg_port->read_blob(CONFIG_GROUP_SCHEDULE,
                                          CONFIG_KEY_SCHEDULE_SLOTS,
                                          &stored,
                                          sizeof(stored),
                                          &blob_len));
    TEST_ASSERT_EQUAL_size_t(sizeof(stored), blob_len);
    TEST_ASSERT_EQUAL_UINT32(SCHEDULE_NVDM_CONFIG_MAGIC, stored.magic);
    TEST_ASSERT_EQUAL_UINT8(SCHEDULE_NVDM_CONFIG_VERSION, stored.version);
    TEST_ASSERT_EQUAL_UINT8(0, stored.count);
}

void test_schedule_format_state_json_slim_wire(void)
{
    schedule_slot_config_t slot = {
        .hour = 8,
        .min = 0,
        .days = 5,
        .g = 30,
        .enabled = true,
    };
    char buf[512];
    int written;

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));

    written = schedule_format_state_json(buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"enabled\":true"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"today_enabled\":true"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"time\":\"08:00\""));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"repeat_days\":[0,2]"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"g\":30"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state\":\"pending\""));
    TEST_ASSERT_NULL(strstr(buf, "\"id\":"));
    TEST_ASSERT_NULL(strstr(buf, "\"days\":"));
    TEST_ASSERT_NULL(strstr(buf, "\"recurring\":"));
}

void test_schedule_set_slot_empty_repeat_days(void)
{
    schedule_slot_config_t slot = {
        .hour = 10,
        .min = 0,
        .days = 0,
        .g = 30,
        .enabled = true,
    };
    schedule_slot_config_t loaded;

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));
    TEST_ASSERT_TRUE(schedule_get_slot(0, &loaded, NULL));
    TEST_ASSERT_EQUAL_UINT8(0, loaded.days);
}

void test_schedule_skip_slot_updates_runtime(void)
{
    schedule_slot_config_t slot = {
        .hour = 9,
        .min = 0,
        .days = 127,
        .g = 25,
        .enabled = true,
    };
    schedule_slot_runtime_t rt;
    char buf[512];

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));
    TEST_ASSERT_TRUE(schedule_skip_slot(9, 0, true));

    TEST_ASSERT_TRUE(schedule_get_slot(0, NULL, &rt));
    TEST_ASSERT_TRUE(rt.skip_today);

    schedule_poll(1000u);
    TEST_ASSERT_GREATER_THAN(0, schedule_format_state_json(buf, sizeof(buf)));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"state\":\"to_be_skipped\""));
}

void test_schedule_global_enable_toggle(void)
{
    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);

    TEST_ASSERT_TRUE(schedule_global_enabled());
    TEST_ASSERT_TRUE(schedule_set_global_enabled(false));
    TEST_ASSERT_FALSE(schedule_global_enabled());
    TEST_ASSERT_TRUE(schedule_set_global_enabled(false));
}

void test_schedule_delete_slot(void)
{
    schedule_slot_config_t slot = {
        .hour = 12,
        .min = 15,
        .days = 127,
        .g = 40,
        .enabled = true,
    };

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));
    TEST_ASSERT_EQUAL_size_t(1, schedule_slot_count());
    TEST_ASSERT_TRUE(schedule_delete_slot(12, 15));
    TEST_ASSERT_EQUAL_size_t(0, schedule_slot_count());
}

void test_schedule_compute_next_json(void)
{
    schedule_slot_config_t slot = {
        .hour = 8,
        .min = 0,
        .days = 127,
        .g = 30,
        .enabled = true,
    };
    char buf[128];
    int written;

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_00_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));

    written = schedule_format_next_json(buf, sizeof(buf));
    TEST_ASSERT_GREATER_THAN(0, written);
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"hour\":8"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"min\":0"));
    TEST_ASSERT_NOT_NULL(strstr(buf, "\"g\":30"));
}

void test_schedule_poll_fires_slot_at_minute(void)
{
    schedule_slot_config_t slot = {
        .hour = 8,
        .min = 0,
        .days = 127,
        .g = 30,
        .enabled = true,
    };
    schedule_slot_runtime_t rt;

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_08_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));

    schedule_poll(1000u);

    TEST_ASSERT_EQUAL_UINT(1, s_fire_calls);
    TEST_ASSERT_EQUAL_UINT8(8, s_fire_hour);
    TEST_ASSERT_EQUAL_UINT8(0, s_fire_min);
    TEST_ASSERT_EQUAL_UINT8(30, s_fire_g);
    TEST_ASSERT_TRUE(schedule_get_slot(0, NULL, &rt));
    TEST_ASSERT_EQUAL(SCHEDULE_STATE_DISPENSING, rt.state);
    TEST_ASSERT_TRUE(rt.fired_today);
}

void test_schedule_poll_marks_past_due_skipped_on_first_tick(void)
{
    schedule_slot_config_t slot = {
        .hour = 7,
        .min = 0,
        .days = 127,
        .g = 30,
        .enabled = true,
    };
    schedule_slot_runtime_t rt;

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_08_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));

    schedule_poll(1000u);

    TEST_ASSERT_EQUAL_UINT(0, s_fire_calls);
    TEST_ASSERT_TRUE(schedule_get_slot(0, NULL, &rt));
    TEST_ASSERT_EQUAL(SCHEDULE_STATE_SKIPPED, rt.state);
}

void test_schedule_poll_busy_fire_skipped_on_next_minute(void)
{
    schedule_slot_config_t slot = {
        .hour = 8,
        .min = 0,
        .days = 127,
        .g = 30,
        .enabled = true,
    };
    schedule_slot_runtime_t rt;

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_08_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));
    s_fire_result = SCHEDULE_FIRE_BUSY;

    schedule_poll(1000u);

    TEST_ASSERT_EQUAL_UINT(1, s_fire_calls);
    TEST_ASSERT_TRUE(schedule_get_slot(0, NULL, &rt));
    TEST_ASSERT_EQUAL(SCHEDULE_STATE_PENDING, rt.state);
    TEST_ASSERT_FALSE(rt.fired_today);

    fake_time_port_set_epoch(SCHEDULE_TEST_EPOCH_THU_08_00_UTC + 60LL);
    schedule_poll(2000u);

    TEST_ASSERT_TRUE(schedule_get_slot(0, NULL, &rt));
    TEST_ASSERT_EQUAL(SCHEDULE_STATE_SKIPPED, rt.state);
}

void test_schedule_on_dispense_complete_success(void)
{
    schedule_slot_config_t slot = {
        .hour = 8,
        .min = 0,
        .days = 127,
        .g = 30,
        .enabled = true,
    };
    schedule_dispense_result_t result = {
        .hour = 8,
        .min = 0,
        .grams = 28,
        .outcome = SCHEDULE_DISPENSE_OK,
    };
    schedule_slot_runtime_t rt;

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_08_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));
    schedule_poll(1000u);

    schedule_on_dispense_complete(&result);

    TEST_ASSERT_TRUE(schedule_get_slot(0, NULL, &rt));
    TEST_ASSERT_EQUAL(SCHEDULE_STATE_DISPENSED, rt.state);
    TEST_ASSERT_EQUAL_INT16(28, rt.g_actual);
}

void test_schedule_on_dispense_complete_failed(void)
{
    schedule_slot_config_t slot = {
        .hour = 8,
        .min = 0,
        .days = 127,
        .g = 30,
        .enabled = true,
    };
    schedule_dispense_result_t result = {
        .hour = 8,
        .min = 0,
        .grams = 0,
        .outcome = SCHEDULE_DISPENSE_FAILED,
    };
    schedule_slot_runtime_t rt;

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_08_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));
    schedule_poll(1000u);
    schedule_on_dispense_complete(&result);

    TEST_ASSERT_TRUE(schedule_get_slot(0, NULL, &rt));
    TEST_ASSERT_EQUAL(SCHEDULE_STATE_FAILED, rt.state);
}

void test_schedule_poll_skipped_full_sets_terminal_state(void)
{
    schedule_slot_config_t slot = {
        .hour = 8,
        .min = 0,
        .days = 127,
        .g = 30,
        .enabled = true,
    };
    schedule_slot_runtime_t rt;

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_08_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));
    s_fire_result = SCHEDULE_FIRE_SKIPPED_FULL;

    schedule_poll(1000u);

    TEST_ASSERT_EQUAL_UINT(1, s_fire_calls);
    TEST_ASSERT_TRUE(schedule_get_slot(0, NULL, &rt));
    TEST_ASSERT_EQUAL(SCHEDULE_STATE_SKIPPED_FULL, rt.state);
    TEST_ASSERT_TRUE(rt.fired_today);
    TEST_ASSERT_EQUAL_STRING("skipped_full", schedule_state_wire(rt.state));
}

void test_schedule_poll_overfill_skips_when_bowl_full(void)
{
    schedule_slot_config_t slot = {
        .hour = 8,
        .min = 0,
        .days = 127,
        .g = 30,
        .enabled = true,
    };
    schedule_slot_runtime_t rt;

    fake_weight_port_reset();
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    app_test_reset();
    app_test_finish_weight_boot();

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_08_00_UTC);
    schedule_set_fire_fn(test_schedule_fire_with_overfill);
    TEST_ASSERT_TRUE(feed_config_overfill_enabled_set(true));
    TEST_ASSERT_TRUE(feed_config_overfill_threshold_g_set(50u));
    app_bowl_dg_notify_read(WEIGHT_G_TO_DG(50), true, 1000u);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));

    schedule_poll(1000u);

    TEST_ASSERT_EQUAL_UINT(1, s_fire_calls);
    TEST_ASSERT_TRUE(schedule_get_slot(0, NULL, &rt));
    TEST_ASSERT_EQUAL(SCHEDULE_STATE_SKIPPED_FULL, rt.state);
    TEST_ASSERT_TRUE(rt.fired_today);
}

void test_schedule_skipped_full_stays_terminal_on_later_poll(void)
{
    schedule_slot_config_t slot = {
        .hour = 8,
        .min = 0,
        .days = 127,
        .g = 30,
        .enabled = true,
    };
    schedule_slot_runtime_t rt;

    setup_schedule_synced(SCHEDULE_TEST_EPOCH_THU_08_00_UTC);
    TEST_ASSERT_TRUE(schedule_set_slot(&slot));
    s_fire_result = SCHEDULE_FIRE_SKIPPED_FULL;

    schedule_poll(1000u);

    fake_time_port_set_epoch(SCHEDULE_TEST_EPOCH_THU_08_00_UTC + 60LL);
    schedule_poll(2000u);

    TEST_ASSERT_TRUE(schedule_get_slot(0, NULL, &rt));
    TEST_ASSERT_EQUAL(SCHEDULE_STATE_SKIPPED_FULL, rt.state);
    TEST_ASSERT_TRUE(rt.fired_today);
}
