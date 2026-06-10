/* Tests: spec/30-processes/button-handling.md § Gesture state machine */

#include "unity.h"

#include "button_gesture.h"
#include "button_input.h"

static void feed_down(button_id_t id, uint32_t at_ms)
{
    button_transition_t tr = {
        .id = id,
        .edge = BUTTON_EDGE_DOWN,
        .at_ms = at_ms,
    };

    button_gesture_on_transition(&tr);
}

static void feed_up(button_id_t id, uint32_t at_ms)
{
    button_transition_t tr = {
        .id = id,
        .edge = BUTTON_EDGE_UP,
        .at_ms = at_ms,
    };

    button_gesture_on_transition(&tr);
}

void test_button_gesture_short_on_release(void)
{
    button_gesture_event_t ev;

    button_gesture_reset();
    feed_down(BUTTON_ID_DISPENSE, 0u);
    button_gesture_step(500u);
    feed_up(BUTTON_ID_DISPENSE, 500u);

    TEST_ASSERT_TRUE(button_gesture_pop(&ev));
    TEST_ASSERT_EQUAL(BUTTON_ID_DISPENSE, ev.id);
    TEST_ASSERT_EQUAL(BUTTON_GESTURE_SHORT, ev.kind);
    TEST_ASSERT_FALSE(button_gesture_pop(&ev));
}

void test_button_gesture_long_while_held(void)
{
    button_gesture_event_t ev;

    button_gesture_reset();
    feed_down(BUTTON_ID_POWER, 0u);
    button_gesture_step(BUTTON_GESTURE_POWER_LONG_MS);

    TEST_ASSERT_TRUE(button_gesture_pop(&ev));
    TEST_ASSERT_EQUAL(BUTTON_ID_POWER, ev.id);
    TEST_ASSERT_EQUAL(BUTTON_GESTURE_LONG, ev.kind);

    feed_up(BUTTON_ID_POWER, BUTTON_GESTURE_POWER_LONG_MS + 100u);
    TEST_ASSERT_FALSE(button_gesture_pop(&ev));
}

void test_button_gesture_long_suppresses_short(void)
{
    button_gesture_event_t ev;

    button_gesture_reset();
    feed_down(BUTTON_ID_RESET, 0u);
    button_gesture_step(BUTTON_GESTURE_RESET_LONG_MS);
    TEST_ASSERT_TRUE(button_gesture_pop(&ev));
    TEST_ASSERT_EQUAL(BUTTON_GESTURE_LONG, ev.kind);

    feed_up(BUTTON_ID_RESET, BUTTON_GESTURE_RESET_LONG_MS + 50u);
    TEST_ASSERT_FALSE(button_gesture_pop(&ev));
}

void test_button_gesture_child_lock_combo(void)
{
    button_gesture_event_t ev;

    button_gesture_reset();
    feed_down(BUTTON_ID_RESET, 0u);
    feed_down(BUTTON_ID_DISPENSE, 10u);
    button_gesture_step(BUTTON_GESTURE_CHILD_LOCK_MS + 10u);

    TEST_ASSERT_TRUE(button_gesture_pop(&ev));
    TEST_ASSERT_EQUAL(BUTTON_GESTURE_CHILD_LOCK_TOGGLE, ev.kind);
    TEST_ASSERT_TRUE(button_gesture_pop(&ev));
    TEST_ASSERT_EQUAL(BUTTON_ID_DISPENSE, ev.id);
    TEST_ASSERT_EQUAL(BUTTON_GESTURE_LONG, ev.kind);
}

void test_button_gesture_dispense_long_threshold(void)
{
    button_gesture_event_t ev;

    button_gesture_reset();
    feed_down(BUTTON_ID_DISPENSE, 0u);
    button_gesture_step(BUTTON_GESTURE_DISPENSE_LONG_MS - 1u);
    TEST_ASSERT_FALSE(button_gesture_pop(&ev));

    button_gesture_step(BUTTON_GESTURE_DISPENSE_LONG_MS);
    TEST_ASSERT_TRUE(button_gesture_pop(&ev));
    TEST_ASSERT_EQUAL(BUTTON_GESTURE_LONG, ev.kind);
}
