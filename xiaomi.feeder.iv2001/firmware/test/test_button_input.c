/* Tests: spec/30-processes/button-handling.md § Software debounce */

#include "unity.h"

#include "button_input.h"
#include "fake_button_port.h"

static void set_pressed(bool power, bool reset, bool dispense)
{
    button_sample_t sample = {
        .power_pressed = power,
        .reset_pressed = reset,
        .dispense_pressed = dispense,
    };

    fake_button_port_set_sample(&sample);
}

static void release_all(void)
{
    set_pressed(false, false, false);
}

static void poll_twice(uint32_t t0, uint32_t t1)
{
    button_input_poll(t0);
    button_input_poll(t1);
}

static bool pop_down(button_id_t id, button_transition_t *tr)
{
    if (!button_input_pop_transition(tr)) {
        return false;
    }

    return tr->edge == BUTTON_EDGE_DOWN && tr->id == id;
}

void test_button_input_dispense_down_after_debounce(void)
{
    button_transition_t tr;

    fake_button_port_reset();
    button_input_init(fake_button_port_get());

    set_pressed(false, false, true);
    poll_twice(0u, 50u);

    TEST_ASSERT_TRUE(pop_down(BUTTON_ID_DISPENSE, &tr));
    TEST_ASSERT_EQUAL_UINT32(50u, tr.at_ms);
    TEST_ASSERT_FALSE(button_input_pop_transition(&tr));
}

void test_button_input_reset_down_polled_without_irq(void)
{
    button_transition_t tr;

    fake_button_port_reset();
    button_input_init(fake_button_port_get());

    set_pressed(false, true, false);
    poll_twice(0u, 50u);

    TEST_ASSERT_TRUE(pop_down(BUTTON_ID_RESET, &tr));
}

void test_button_input_power_down_and_up_edges(void)
{
    button_transition_t tr;

    fake_button_port_reset();
    button_input_init(fake_button_port_get());

    set_pressed(true, false, false);
    poll_twice(0u, 50u);
    TEST_ASSERT_TRUE(pop_down(BUTTON_ID_POWER, &tr));

    poll_twice(100u, 150u);
    TEST_ASSERT_FALSE(button_input_pop_transition(&tr));

    release_all();
    poll_twice(200u, 250u);
    TEST_ASSERT_TRUE(button_input_pop_transition(&tr));
    TEST_ASSERT_EQUAL(BUTTON_EDGE_UP, tr.edge);
    TEST_ASSERT_EQUAL(BUTTON_ID_POWER, tr.id);
}

void test_button_input_bounce_rejects_unstable_reads(void)
{
    button_transition_t tr;

    fake_button_port_reset();
    button_input_init(fake_button_port_get());

    set_pressed(true, false, false);
    button_input_poll(0u);
    release_all();
    button_input_poll(25u);
    set_pressed(true, false, false);
    button_input_poll(50u);
    release_all();
    button_input_poll(75u);
    TEST_ASSERT_FALSE(button_input_pop_transition(&tr));

    set_pressed(true, false, false);
    poll_twice(100u, 150u);
    TEST_ASSERT_TRUE(pop_down(BUTTON_ID_POWER, &tr));
}

void test_button_input_irq_debounce_blocks_irq_buttons_not_reset(void)
{
    button_transition_t tr;

    fake_button_port_reset();
    button_input_init(fake_button_port_get());

    button_input_notify_irq(0u);

    set_pressed(true, true, false);
    button_input_poll(25u);
    TEST_ASSERT_FALSE(button_input_pop_transition(&tr));

    set_pressed(false, true, false);
    poll_twice(50u, 100u);
    TEST_ASSERT_TRUE(pop_down(BUTTON_ID_RESET, &tr));
    TEST_ASSERT_FALSE(button_input_pop_transition(&tr));

    set_pressed(true, false, false);
    poll_twice(150u, 200u);
    TEST_ASSERT_TRUE(pop_down(BUTTON_ID_POWER, &tr));
}

void test_button_input_read_failure_is_silent(void)
{
    button_transition_t tr;

    fake_button_port_reset();
    button_input_init(fake_button_port_get());

    set_pressed(true, false, false);
    fake_button_port_set_read_err(PORT_ERR_IO);
    poll_twice(0u, 50u);
    TEST_ASSERT_FALSE(button_input_pop_transition(&tr));
}

void test_button_input_dispense_active_high_semantics_via_port(void)
{
    button_transition_t tr;

    fake_button_port_reset();
    button_input_init(fake_button_port_get());

    set_pressed(false, false, true);
    poll_twice(0u, 50u);
    TEST_ASSERT_TRUE(pop_down(BUTTON_ID_DISPENSE, &tr));

    set_pressed(false, false, false);
    poll_twice(100u, 150u);
    TEST_ASSERT_TRUE(button_input_pop_transition(&tr));
    TEST_ASSERT_EQUAL(BUTTON_EDGE_UP, tr.edge);
}
