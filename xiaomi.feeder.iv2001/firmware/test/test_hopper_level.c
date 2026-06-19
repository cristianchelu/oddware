/* Tests: spec/30-processes/hopper-sensing.md § hopper_level */

#include "unity.h"

#include "dispense.h"
#include "fake_hopper_ir_port.h"
#include "hopper_input.h"
#include "hopper_level.h"

static void set_ir_low(uint32_t t0)
{
    uint32_t t = t0;
    uint8_t i;

    fake_hopper_ir_port_set_beam_blocked(false);
    hopper_input_notify_dispense_complete();
    hopper_input_poll(t, true);
    t += HOPPER_INPUT_DEBOUNCE_INTERVAL_MS;

    for (i = 1u; i < HOPPER_INPUT_LOW_STREAK_REQUIRED; i++) {
        hopper_input_poll(t, true);
        t += HOPPER_INPUT_DEBOUNCE_INTERVAL_MS;
    }
}

static void set_ir_normal_from_low(uint32_t t0)
{
    uint32_t t = t0;
    uint8_t i;

    fake_hopper_ir_port_set_beam_blocked(true);
    hopper_input_notify_dispense_complete();
    hopper_input_poll(t, true);
    t += HOPPER_INPUT_DEBOUNCE_INTERVAL_MS;

    for (i = 1u; i < HOPPER_INPUT_NORMAL_STREAK_REQUIRED; i++) {
        hopper_input_poll(t, true);
        t += HOPPER_INPUT_DEBOUNCE_INTERVAL_MS;
    }

    hopper_level_poll();
}

static void hopper_level_test_reset(void)
{
    fake_hopper_ir_port_reset();
    hopper_input_init(hopper_ir_port_get());
    hopper_level_init();
}

void test_hopper_level_starts_normal(void)
{
    hopper_level_test_reset();
    TEST_ASSERT_EQUAL(HOPPER_LEVEL_STATE_NORMAL, hopper_level_get());
}

void test_hopper_level_ir_low_publishes_low(void)
{
    hopper_level_state_transition_t tr;

    hopper_level_test_reset();
    set_ir_low(0u);
    hopper_level_poll();

    TEST_ASSERT_EQUAL(HOPPER_LEVEL_STATE_LOW, hopper_level_get());
    TEST_ASSERT_TRUE(hopper_level_pop_transition(&tr));
    TEST_ASSERT_EQUAL(HOPPER_LEVEL_STATE_LOW, tr.level);
}

void test_hopper_level_latches_empty_on_zero_delta_with_ir_low(void)
{
    hopper_level_state_transition_t tr;

    hopper_level_test_reset();
    set_ir_low(0u);
    hopper_level_poll();
    while (hopper_level_pop_transition(&tr)) {
    }

    TEST_ASSERT_EQUAL(
        DISPENSE_OUTCOME_EMPTY_HOPPER,
        hopper_level_on_dispense_finished(DISPENSE_OUTCOME_SUCCESS, 0, true, 6000u));

    TEST_ASSERT_EQUAL(HOPPER_LEVEL_STATE_EMPTY, hopper_level_get());
    TEST_ASSERT_TRUE(hopper_level_pop_transition(&tr));
    TEST_ASSERT_EQUAL(HOPPER_LEVEL_STATE_EMPTY, tr.level);
}

void test_hopper_level_no_empty_latch_on_stuck(void)
{
    hopper_level_test_reset();
    set_ir_low(0u);
    hopper_level_poll();

    TEST_ASSERT_EQUAL(
        DISPENSE_OUTCOME_STUCK,
        hopper_level_on_dispense_finished(DISPENSE_OUTCOME_STUCK, 0, true, 6000u));

    TEST_ASSERT_EQUAL(HOPPER_LEVEL_STATE_LOW, hopper_level_get());
}

void test_hopper_level_no_empty_latch_on_zero_delta_when_ir_normal(void)
{
    hopper_level_test_reset();
    TEST_ASSERT_EQUAL(
        DISPENSE_OUTCOME_SUCCESS,
        hopper_level_on_dispense_finished(DISPENSE_OUTCOME_SUCCESS, 0, true, 1000u));

    TEST_ASSERT_EQUAL(HOPPER_LEVEL_STATE_NORMAL, hopper_level_get());
}

void test_hopper_level_positive_delta_clears_empty_latch(void)
{
    hopper_level_state_transition_t tr;

    hopper_level_test_reset();
    set_ir_low(0u);
    hopper_level_poll();
    (void)hopper_level_on_dispense_finished(DISPENSE_OUTCOME_SUCCESS, 0, true, 6000u);
    while (hopper_level_pop_transition(&tr)) {
    }

    TEST_ASSERT_EQUAL(HOPPER_LEVEL_STATE_EMPTY, hopper_level_get());

    TEST_ASSERT_EQUAL(
        DISPENSE_OUTCOME_SUCCESS,
        hopper_level_on_dispense_finished(DISPENSE_OUTCOME_SUCCESS, 12, true, 7000u));

    TEST_ASSERT_EQUAL(HOPPER_LEVEL_STATE_LOW, hopper_level_get());
}

void test_hopper_level_refill_clears_empty_latch(void)
{
    hopper_level_state_transition_t tr;

    hopper_level_test_reset();
    set_ir_low(0u);
    hopper_level_poll();
    (void)hopper_level_on_dispense_finished(DISPENSE_OUTCOME_SUCCESS, 0, true, 6000u);
    while (hopper_level_pop_transition(&tr)) {
    }

    set_ir_normal_from_low(10000u);

    TEST_ASSERT_EQUAL(HOPPER_LEVEL_STATE_NORMAL, hopper_level_get());
    TEST_ASSERT_TRUE(hopper_level_pop_transition(&tr));
    TEST_ASSERT_EQUAL(HOPPER_LEVEL_STATE_NORMAL, tr.level);
}

void test_hopper_level_empty_suppresses_low_transition(void)
{
    hopper_level_state_transition_t tr;

    hopper_level_test_reset();
    set_ir_low(0u);
    hopper_level_poll();
    while (hopper_level_pop_transition(&tr)) {
    }

    (void)hopper_level_on_dispense_finished(DISPENSE_OUTCOME_SUCCESS, 0, true, 6000u);
    while (hopper_level_pop_transition(&tr)) {
    }
    TEST_ASSERT_EQUAL(HOPPER_LEVEL_STATE_EMPTY, hopper_level_get());

    hopper_level_poll();
    TEST_ASSERT_FALSE(hopper_level_pop_transition(&tr));
}

void test_hopper_level_underfill_latches_empty(void)
{
    hopper_level_state_transition_t tr;

    hopper_level_test_reset();
    TEST_ASSERT_EQUAL(
        DISPENSE_OUTCOME_UNDERFILL,
        hopper_level_on_dispense_finished(DISPENSE_OUTCOME_UNDERFILL, 5, true, 1000u));

    TEST_ASSERT_EQUAL(HOPPER_LEVEL_STATE_EMPTY, hopper_level_get());
    TEST_ASSERT_TRUE(hopper_level_pop_transition(&tr));
    TEST_ASSERT_EQUAL(HOPPER_LEVEL_STATE_EMPTY, tr.level);
}
