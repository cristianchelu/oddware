/* Tests: spec/30-processes/power-state-machine.md § Mains sense input */

#include <string.h>

#include "unity.h"

#include "app_log.h"
#include "fake_power_source_port.h"
#include "power_source_input.h"

static void set_mains(bool present)
{
    fake_power_source_port_set_mains_present(present);
}

static void poll_twice(uint32_t t0, uint32_t t1)
{
    power_source_input_poll(t0);
    power_source_input_poll(t1);
}

static bool pop_edge(power_source_edge_t edge, power_source_transition_t *tr)
{
    if (!power_source_input_pop_transition(tr)) {
        return false;
    }

    return tr->edge == edge;
}

void test_power_source_input_boot_seeds_mains_without_transition(void)
{
    power_source_transition_t tr;

    fake_power_source_port_reset();
    set_mains(true);
    power_source_input_init(fake_power_source_port_get());

    TEST_ASSERT_TRUE(power_source_input_is_valid());
    TEST_ASSERT_EQUAL(POWER_SOURCE_MAINS, power_source_input_get());
    TEST_ASSERT_FALSE(power_source_input_pop_transition(&tr));
}

void test_power_source_input_boot_seeds_battery_without_transition(void)
{
    power_source_transition_t tr;

    fake_power_source_port_reset();
    set_mains(false);
    power_source_input_init(fake_power_source_port_get());

    TEST_ASSERT_TRUE(power_source_input_is_valid());
    TEST_ASSERT_EQUAL(POWER_SOURCE_BATTERY, power_source_input_get());
    TEST_ASSERT_FALSE(power_source_input_pop_transition(&tr));
}

void test_power_source_input_irq_gate_blocks_sample_for_50_ms(void)
{
    power_source_transition_t tr;

    fake_power_source_port_reset();
    set_mains(false);
    power_source_input_init(fake_power_source_port_get());
    power_source_input_notify_irq(0u);

    set_mains(true);
    power_source_input_poll(49u);
    TEST_ASSERT_EQUAL(POWER_SOURCE_BATTERY, power_source_input_get());
    TEST_ASSERT_FALSE(power_source_input_pop_transition(&tr));

    poll_twice(50u, 100u);
    TEST_ASSERT_TRUE(pop_edge(POWER_SOURCE_EDGE_MAINS, &tr));
    TEST_ASSERT_EQUAL_UINT32(100u, tr.at_ms);
}

void test_power_source_input_bounce_rejects_unstable_reads(void)
{
    power_source_transition_t tr;

    fake_power_source_port_reset();
    set_mains(true);
    power_source_input_init(fake_power_source_port_get());
    power_source_input_notify_irq(100u);

    set_mains(false);
    power_source_input_poll(150u);
    set_mains(true);
    power_source_input_poll(175u);
    set_mains(false);
    poll_twice(200u, 250u);
    TEST_ASSERT_TRUE(pop_edge(POWER_SOURCE_EDGE_BATTERY, &tr));
}

void test_power_source_input_stable_mains_to_battery_transition(void)
{
    power_source_transition_t tr;

    fake_power_source_port_reset();
    set_mains(true);
    power_source_input_init(fake_power_source_port_get());
    power_source_input_notify_irq(0u);

    set_mains(false);
    poll_twice(50u, 100u);

    TEST_ASSERT_EQUAL(POWER_SOURCE_BATTERY, power_source_input_get());
    TEST_ASSERT_TRUE(pop_edge(POWER_SOURCE_EDGE_BATTERY, &tr));
    TEST_ASSERT_EQUAL_UINT32(100u, tr.at_ms);
}

void test_power_source_input_stable_battery_to_mains_transition(void)
{
    power_source_transition_t tr;

    fake_power_source_port_reset();
    set_mains(false);
    power_source_input_init(fake_power_source_port_get());
    power_source_input_notify_irq(0u);

    set_mains(true);
    poll_twice(50u, 100u);

    TEST_ASSERT_EQUAL(POWER_SOURCE_MAINS, power_source_input_get());
    TEST_ASSERT_TRUE(pop_edge(POWER_SOURCE_EDGE_MAINS, &tr));
}

void test_power_source_input_boot_read_failure_marks_invalid(void)
{
    fake_power_source_port_reset();
    fake_power_source_port_set_read_err(PORT_ERR_IO);
    power_source_input_init(fake_power_source_port_get());

    TEST_ASSERT_FALSE(power_source_input_is_valid());

    fake_power_source_port_set_read_err(PORT_OK);
    set_mains(true);
    poll_twice(0u, 1u);
    TEST_ASSERT_TRUE(power_source_input_is_valid());
    TEST_ASSERT_EQUAL(POWER_SOURCE_MAINS, power_source_input_get());
}

void test_power_source_input_read_failure_preserves_confirmed_state(void)
{
    power_source_transition_t tr;

    fake_power_source_port_reset();
    set_mains(true);
    power_source_input_init(fake_power_source_port_get());
    power_source_input_notify_irq(0u);

    set_mains(false);
    fake_power_source_port_set_read_err(PORT_ERR_IO);
    poll_twice(50u, 100u);

    TEST_ASSERT_EQUAL(POWER_SOURCE_MAINS, power_source_input_get());
    TEST_ASSERT_FALSE(power_source_input_pop_transition(&tr));
}

void test_power_source_input_transition_logs_mains_connected(void)
{
    power_source_transition_t tr;

    fake_power_source_port_reset();
    set_mains(false);
    power_source_input_init(fake_power_source_port_get());
    power_source_input_notify_irq(0u);

    app_log_test_reset();
    set_mains(true);
    poll_twice(50u, 100u);
    TEST_ASSERT_TRUE(pop_edge(POWER_SOURCE_EDGE_MAINS, &tr));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "[power]"));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "mains connected"));
}

void test_power_source_input_transition_logs_mains_lost(void)
{
    power_source_transition_t tr;

    fake_power_source_port_reset();
    set_mains(true);
    power_source_input_init(fake_power_source_port_get());
    power_source_input_notify_irq(0u);

    app_log_test_reset();
    set_mains(false);
    poll_twice(50u, 100u);
    TEST_ASSERT_TRUE(pop_edge(POWER_SOURCE_EDGE_BATTERY, &tr));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "[power]"));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "mains lost"));
}

/* Regression: UART log and confirmed state update when transition queue is full. */
void test_power_source_input_logs_mains_after_queue_fills_without_pop(void)
{
    fake_power_source_port_reset();
    set_mains(false);
    power_source_input_init(fake_power_source_port_get());

    power_source_input_notify_irq(0u);
    set_mains(true);
    poll_twice(50u, 100u);

    power_source_input_notify_irq(200u);
    set_mains(false);
    poll_twice(250u, 300u);

    power_source_input_notify_irq(400u);
    set_mains(true);
    poll_twice(450u, 500u);

    power_source_input_notify_irq(600u);
    set_mains(false);
    poll_twice(650u, 700u);

    app_log_test_reset();
    power_source_input_notify_irq(800u);
    set_mains(true);
    poll_twice(850u, 900u);

    TEST_ASSERT_EQUAL(POWER_SOURCE_MAINS, power_source_input_get());
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "[power]"));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "mains connected"));
}

void test_power_source_input_display_tick_poll_without_irq(void)
{
    fake_power_source_port_reset();
    set_mains(false);
    power_source_input_init(fake_power_source_port_get());

    set_mains(true);
    poll_twice(1000u, 1001u);

    TEST_ASSERT_EQUAL(POWER_SOURCE_MAINS, power_source_input_get());
}
