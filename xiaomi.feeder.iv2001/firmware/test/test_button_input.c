/* Tests: spec/30-processes/button-handling.md § Bring-up UART logging */

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

void test_button_input_dispense_press_after_debounce(void)
{
    button_id_t id;

    fake_button_port_reset();
    button_input_init(fake_button_port_get());

    set_pressed(false, false, true);
    poll_twice(0u, 50u);

    TEST_ASSERT_TRUE(button_input_pop_press(&id));
    TEST_ASSERT_EQUAL(BUTTON_ID_DISPENSE, id);
    TEST_ASSERT_FALSE(button_input_pop_press(&id));
}

void test_button_input_reset_press_polled_without_irq(void)
{
    button_id_t id;

    fake_button_port_reset();
    button_input_init(fake_button_port_get());

    set_pressed(false, true, false);
    poll_twice(0u, 50u);

    TEST_ASSERT_TRUE(button_input_pop_press(&id));
    TEST_ASSERT_EQUAL(BUTTON_ID_RESET, id);
}

void test_button_input_power_press_edge_only(void)
{
    button_id_t id;

    fake_button_port_reset();
    button_input_init(fake_button_port_get());

    set_pressed(true, false, false);
    poll_twice(0u, 50u);
    TEST_ASSERT_TRUE(button_input_pop_press(&id));
    TEST_ASSERT_EQUAL(BUTTON_ID_POWER, id);

    poll_twice(100u, 150u);
    TEST_ASSERT_FALSE(button_input_pop_press(&id));

    release_all();
    poll_twice(200u, 250u);
    set_pressed(true, false, false);
    poll_twice(300u, 350u);
    TEST_ASSERT_TRUE(button_input_pop_press(&id));
    TEST_ASSERT_EQUAL(BUTTON_ID_POWER, id);
}

void test_button_input_bounce_rejects_unstable_reads(void)
{
    button_id_t id;

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
    TEST_ASSERT_FALSE(button_input_pop_press(&id));

    set_pressed(true, false, false);
    poll_twice(100u, 150u);
    TEST_ASSERT_TRUE(button_input_pop_press(&id));
    TEST_ASSERT_EQUAL(BUTTON_ID_POWER, id);
}

void test_button_input_irq_debounce_blocks_irq_buttons_not_reset(void)
{
    button_id_t id;

    fake_button_port_reset();
    button_input_init(fake_button_port_get());

    button_input_notify_irq(0u);

    set_pressed(true, true, false);
    button_input_poll(25u);
    TEST_ASSERT_FALSE(button_input_pop_press(&id));

    set_pressed(false, true, false);
    poll_twice(50u, 100u);
    TEST_ASSERT_TRUE(button_input_pop_press(&id));
    TEST_ASSERT_EQUAL(BUTTON_ID_RESET, id);
    TEST_ASSERT_FALSE(button_input_pop_press(&id));

    set_pressed(true, false, false);
    poll_twice(150u, 200u);
    TEST_ASSERT_TRUE(button_input_pop_press(&id));
    TEST_ASSERT_EQUAL(BUTTON_ID_POWER, id);
}

void test_button_input_read_failure_is_silent(void)
{
    button_id_t id;

    fake_button_port_reset();
    button_input_init(fake_button_port_get());

    set_pressed(true, false, false);
    fake_button_port_set_read_err(PORT_ERR_IO);
    poll_twice(0u, 50u);
    TEST_ASSERT_FALSE(button_input_pop_press(&id));
}
