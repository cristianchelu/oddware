/* Tests: spec/30-processes/hopper-sensing.md */

#include "unity.h"

#include "fake_hopper_ir_port.h"
#include "hopper_input.h"

static void poll_clear_streak(uint32_t t0, uint8_t count, bool background_enabled)
{
    uint32_t t = t0;
    uint8_t i;

    for (i = 0u; i < count; i++) {
        hopper_input_poll(t, background_enabled);
        t += HOPPER_INPUT_DEBOUNCE_INTERVAL_MS;
    }
}

void test_hopper_input_starts_normal(void)
{
    fake_hopper_ir_port_reset();
    hopper_input_init(hopper_ir_port_get());

    TEST_ASSERT_EQUAL(HOPPER_LEVEL_NORMAL, hopper_input_get_level());
    TEST_ASSERT_FALSE(hopper_input_almost_empty());
}

void test_hopper_input_post_dispense_clear_starts_debounce(void)
{
    fake_hopper_ir_port_reset();
    hopper_input_init(hopper_ir_port_get());

    fake_hopper_ir_port_set_beam_blocked(false);
    hopper_input_notify_dispense_complete();
    hopper_input_poll(0u, false);

    TEST_ASSERT_EQUAL(HOPPER_LEVEL_NORMAL, hopper_input_get_level());
    TEST_ASSERT_FALSE(hopper_input_almost_empty());
    TEST_ASSERT_EQUAL_UINT32(1u, fake_hopper_ir_port_sense_count());
}

void test_hopper_input_six_clear_readings_set_almost_empty(void)
{
    hopper_level_transition_t tr;

    fake_hopper_ir_port_reset();
    hopper_input_init(hopper_ir_port_get());

    fake_hopper_ir_port_set_beam_blocked(false);
    hopper_input_notify_dispense_complete();
    poll_clear_streak(0u, HOPPER_INPUT_LOW_STREAK_REQUIRED, true);

    TEST_ASSERT_TRUE(hopper_input_almost_empty());
    TEST_ASSERT_EQUAL(HOPPER_LEVEL_LOW, hopper_input_get_level());
    TEST_ASSERT_TRUE(hopper_input_pop_transition(&tr));
    TEST_ASSERT_EQUAL(HOPPER_LEVEL_LOW, tr.level);
    TEST_ASSERT_EQUAL_UINT32(
        (HOPPER_INPUT_LOW_STREAK_REQUIRED - 1u) * HOPPER_INPUT_DEBOUNCE_INTERVAL_MS,
        tr.at_ms);
}

void test_hopper_input_blocked_reading_resets_low_debounce(void)
{
    fake_hopper_ir_port_reset();
    hopper_input_init(hopper_ir_port_get());

    fake_hopper_ir_port_set_beam_blocked(false);
    hopper_input_notify_dispense_complete();
    poll_clear_streak(0u, 3u, true);

    fake_hopper_ir_port_set_beam_blocked(true);
    hopper_input_poll(3000u, true);

    fake_hopper_ir_port_set_beam_blocked(false);
    hopper_input_notify_dispense_complete();
    hopper_input_poll(3100u, true);
    poll_clear_streak(4100u, HOPPER_INPUT_LOW_STREAK_REQUIRED - 2u, true);

    TEST_ASSERT_FALSE(hopper_input_almost_empty());
}

void test_hopper_input_low_to_normal_requires_three_blocked(void)
{
    hopper_level_transition_t tr;

    fake_hopper_ir_port_reset();
    hopper_input_init(hopper_ir_port_get());

    fake_hopper_ir_port_set_beam_blocked(false);
    hopper_input_notify_dispense_complete();
    poll_clear_streak(0u, HOPPER_INPUT_LOW_STREAK_REQUIRED, true);
    TEST_ASSERT_TRUE(hopper_input_almost_empty());

    while (hopper_input_pop_transition(&tr)) {
    }

    fake_hopper_ir_port_set_beam_blocked(true);
    hopper_input_notify_dispense_complete();
    poll_clear_streak(6000u, HOPPER_INPUT_NORMAL_STREAK_REQUIRED, true);

    TEST_ASSERT_FALSE(hopper_input_almost_empty());
    TEST_ASSERT_EQUAL(HOPPER_LEVEL_NORMAL, hopper_input_get_level());
    TEST_ASSERT_TRUE(hopper_input_pop_transition(&tr));
    TEST_ASSERT_EQUAL(HOPPER_LEVEL_NORMAL, tr.level);
}

void test_hopper_input_debounce_waits_one_second_between_samples(void)
{
    fake_hopper_ir_port_reset();
    hopper_input_init(hopper_ir_port_get());

    fake_hopper_ir_port_set_beam_blocked(false);
    hopper_input_notify_dispense_complete();
    hopper_input_poll(0u, true);
    TEST_ASSERT_EQUAL_UINT32(1u, fake_hopper_ir_port_sense_count());

    hopper_input_poll(500u, true);
    TEST_ASSERT_EQUAL_UINT32(1u, fake_hopper_ir_port_sense_count());

    hopper_input_poll(1000u, true);
    TEST_ASSERT_EQUAL_UINT32(2u, fake_hopper_ir_port_sense_count());
}

void test_hopper_input_background_fires_on_mains_interval(void)
{
    fake_hopper_ir_port_reset();
    hopper_input_init(hopper_ir_port_get());

    hopper_input_poll(0u, true);
    TEST_ASSERT_EQUAL_UINT32(0u, fake_hopper_ir_port_sense_count());

    hopper_input_poll(HOPPER_INPUT_BACKGROUND_INTERVAL_MS - 1u, true);
    TEST_ASSERT_EQUAL_UINT32(0u, fake_hopper_ir_port_sense_count());

    hopper_input_poll(HOPPER_INPUT_BACKGROUND_INTERVAL_MS, true);
    TEST_ASSERT_EQUAL_UINT32(1u, fake_hopper_ir_port_sense_count());
}

void test_hopper_input_background_disabled_on_battery(void)
{
    fake_hopper_ir_port_reset();
    hopper_input_init(hopper_ir_port_get());

    hopper_input_poll(0u, false);
    hopper_input_poll(HOPPER_INPUT_BACKGROUND_INTERVAL_MS, false);

    TEST_ASSERT_EQUAL_UINT32(0u, fake_hopper_ir_port_sense_count());
}
