/* Tests: spec/30-processes/dispense-cycle.md § Motor sequencing */

#include "unity.h"

#include "board_gpio_iv2001.h"
#include "fake_gpio_expander_port.h"
#include "fake_time.h"
#include "motor_driver.h"

static void test_delay_ms(uint32_t ms)
{
    fake_time_advance_ms(ms);
}

static motor_driver_state_t s_state;

static void motor_driver_test_reset(void)
{
    motor_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .delay_ms = test_delay_ms,
    };

    fake_gpio_expander_reset();
    fake_time_reset();
    motor_driver_init(&s_state, &hw);
}

void test_motor_run_forward_sets_ph_before_en_and_auto_stops(void)
{
    motor_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK, motor_driver_run_forward_ms(&s_state, 250u));
    TEST_ASSERT_FALSE(s_state.running);
    TEST_ASSERT_TRUE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_PH_PORT,
                                           BOARD_GPIO_MOTOR_PH_PIN));
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                             BOARD_GPIO_MOTOR_EN_PIN));
    TEST_ASSERT_EQUAL(350u, (unsigned)fake_time_ticks());
    TEST_ASSERT_EQUAL(3u, fake_gpio_expander_set_pin_calls());
}

void test_motor_run_accepts_max_duration(void)
{
    motor_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_driver_run_forward_ms(&s_state, MOTOR_RUN_MS_MAX));
    TEST_ASSERT_EQUAL(MOTOR_PH_SETTLE_MS + MOTOR_RUN_MS_MAX,
                      (unsigned)fake_time_ticks());
}

void test_motor_run_forward_min_duration_1ms(void)
{
    motor_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK, motor_driver_run_forward_ms(&s_state, 1u));
    TEST_ASSERT_EQUAL(MOTOR_PH_SETTLE_MS + 1u, (unsigned)fake_time_ticks());
}

void test_motor_run_rejects_zero_and_over_max_duration(void)
{
    motor_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      motor_driver_run_forward_ms(&s_state, 0u));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      motor_driver_run_forward_ms(&s_state, MOTOR_RUN_MS_MAX + 1u));
}

void test_motor_run_rejects_overlap(void)
{
    motor_driver_test_reset();
    s_state.running = true;
    TEST_ASSERT_EQUAL(PORT_ERR_BUSY, motor_driver_run_forward_ms(&s_state, 100u));
}

void test_motor_run_set_pin_call_order(void)
{
    uint8_t port;
    uint8_t pin;
    bool level;

    motor_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK, motor_driver_run_forward_ms(&s_state, 50u));

    TEST_ASSERT_TRUE(fake_gpio_expander_get_set_pin_log(0u, &port, &pin, &level));
    TEST_ASSERT_EQUAL(BOARD_GPIO_MOTOR_PH_PORT, port);
    TEST_ASSERT_EQUAL(BOARD_GPIO_MOTOR_PH_PIN, pin);
    TEST_ASSERT_TRUE(level);

    TEST_ASSERT_TRUE(fake_gpio_expander_get_set_pin_log(1u, &port, &pin, &level));
    TEST_ASSERT_EQUAL(BOARD_GPIO_MOTOR_EN_PORT, port);
    TEST_ASSERT_EQUAL(BOARD_GPIO_MOTOR_EN_PIN, pin);
    TEST_ASSERT_TRUE(level);

    TEST_ASSERT_TRUE(fake_gpio_expander_get_set_pin_log(2u, &port, &pin, &level));
    TEST_ASSERT_EQUAL(BOARD_GPIO_MOTOR_EN_PORT, port);
    TEST_ASSERT_EQUAL(BOARD_GPIO_MOTOR_EN_PIN, pin);
    TEST_ASSERT_FALSE(level);
}

void test_motor_run_ph_set_failure_leaves_en_low(void)
{
    motor_driver_test_reset();
    fake_gpio_expander_set_set_pin_fail_on(1u, PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, motor_driver_run_forward_ms(&s_state, 50u));
    TEST_ASSERT_FALSE(s_state.running);
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                             BOARD_GPIO_MOTOR_EN_PIN));
    TEST_ASSERT_EQUAL(0u, fake_time_ticks());
}

void test_motor_run_en_set_failure_releases_ph(void)
{
    motor_driver_test_reset();
    fake_gpio_expander_set_set_pin_fail_on(2u, PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, motor_driver_run_forward_ms(&s_state, 50u));
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_PH_PORT,
                                             BOARD_GPIO_MOTOR_PH_PIN));
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                             BOARD_GPIO_MOTOR_EN_PIN));
}

void test_motor_run_retries_coast_stop_after_en_asserted(void)
{
    motor_driver_test_reset();
    fake_gpio_expander_set_set_pin_fail_on(3u, PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_OK, motor_driver_run_forward_ms(&s_state, 50u));
    TEST_ASSERT_EQUAL(4u, fake_gpio_expander_set_pin_calls());
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                             BOARD_GPIO_MOTOR_EN_PIN));
    TEST_ASSERT_TRUE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_PH_PORT,
                                            BOARD_GPIO_MOTOR_PH_PIN));
}

void test_motor_coast_stop_succeeds_on_third_attempt(void)
{
    motor_driver_test_reset();
    fake_gpio_expander_set_set_pin_fail_range(3u, 4u, PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_OK, motor_driver_run_forward_ms(&s_state, 50u));
    TEST_ASSERT_EQUAL(5u, fake_gpio_expander_set_pin_calls());
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                             BOARD_GPIO_MOTOR_EN_PIN));
}

void test_motor_run_coast_stop_exhausted_retries_releases_ph(void)
{
    motor_driver_test_reset();
    fake_gpio_expander_set_set_pin_fail_range(3u, 5u, PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, motor_driver_run_forward_ms(&s_state, 50u));
    TEST_ASSERT_TRUE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                            BOARD_GPIO_MOTOR_EN_PIN));
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_PH_PORT,
                                             BOARD_GPIO_MOTOR_PH_PIN));
}

void test_motor_run_null_delay_still_coast_stops(void)
{
    motor_hw_t hw = {
        .expander = fake_gpio_expander_port_get(),
        .delay_ms = NULL,
    };

    fake_gpio_expander_reset();
    motor_driver_init(&s_state, &hw);

    TEST_ASSERT_EQUAL(PORT_OK, motor_driver_run_forward_ms(&s_state, 50u));
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                             BOARD_GPIO_MOTOR_EN_PIN));
}

void test_motor_run_reverse_sets_ph_low_before_en_and_auto_stops(void)
{
    motor_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK, motor_driver_run_reverse_ms(&s_state, 250u));
    TEST_ASSERT_FALSE(s_state.running);
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_PH_PORT,
                                            BOARD_GPIO_MOTOR_PH_PIN));
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                             BOARD_GPIO_MOTOR_EN_PIN));
    TEST_ASSERT_EQUAL(350u, (unsigned)fake_time_ticks());
    TEST_ASSERT_EQUAL(3u, fake_gpio_expander_set_pin_calls());
}

void test_motor_run_reverse_set_pin_call_order(void)
{
    uint8_t port;
    uint8_t pin;
    bool level;

    motor_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK, motor_driver_run_reverse_ms(&s_state, 50u));

    TEST_ASSERT_TRUE(fake_gpio_expander_get_set_pin_log(0u, &port, &pin, &level));
    TEST_ASSERT_EQUAL(BOARD_GPIO_MOTOR_PH_PORT, port);
    TEST_ASSERT_EQUAL(BOARD_GPIO_MOTOR_PH_PIN, pin);
    TEST_ASSERT_FALSE(level);

    TEST_ASSERT_TRUE(fake_gpio_expander_get_set_pin_log(1u, &port, &pin, &level));
    TEST_ASSERT_EQUAL(BOARD_GPIO_MOTOR_EN_PORT, port);
    TEST_ASSERT_EQUAL(BOARD_GPIO_MOTOR_EN_PIN, pin);
    TEST_ASSERT_TRUE(level);

    TEST_ASSERT_TRUE(fake_gpio_expander_get_set_pin_log(2u, &port, &pin, &level));
    TEST_ASSERT_EQUAL(BOARD_GPIO_MOTOR_EN_PORT, port);
    TEST_ASSERT_EQUAL(BOARD_GPIO_MOTOR_EN_PIN, pin);
    TEST_ASSERT_FALSE(level);
}

void test_motor_run_reverse_rejects_overlap(void)
{
    motor_driver_test_reset();
    s_state.running = true;
    TEST_ASSERT_EQUAL(PORT_ERR_BUSY, motor_driver_run_reverse_ms(&s_state, 100u));
}

void test_motor_run_reverse_ph_set_failure_leaves_en_low(void)
{
    motor_driver_test_reset();
    fake_gpio_expander_set_set_pin_fail_on(1u, PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, motor_driver_run_reverse_ms(&s_state, 50u));
    TEST_ASSERT_FALSE(s_state.running);
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                             BOARD_GPIO_MOTOR_EN_PIN));
    TEST_ASSERT_EQUAL(0u, fake_time_ticks());
}

void test_motor_run_reverse_en_set_failure_releases_ph(void)
{
    motor_driver_test_reset();
    fake_gpio_expander_set_set_pin_fail_on(2u, PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, motor_driver_run_reverse_ms(&s_state, 50u));
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_PH_PORT,
                                             BOARD_GPIO_MOTOR_PH_PIN));
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                             BOARD_GPIO_MOTOR_EN_PIN));
}

void test_motor_run_reverse_retries_coast_stop_after_en_asserted(void)
{
    motor_driver_test_reset();
    fake_gpio_expander_set_set_pin_fail_on(3u, PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_OK, motor_driver_run_reverse_ms(&s_state, 50u));
    TEST_ASSERT_EQUAL(4u, fake_gpio_expander_set_pin_calls());
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                             BOARD_GPIO_MOTOR_EN_PIN));
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_PH_PORT,
                                             BOARD_GPIO_MOTOR_PH_PIN));
}

void test_motor_run_reverse_coast_stop_exhausted_retries_releases_ph(void)
{
    motor_driver_test_reset();
    fake_gpio_expander_set_set_pin_fail_range(3u, 5u, PORT_ERR_IO);

    TEST_ASSERT_EQUAL(PORT_ERR_IO, motor_driver_run_reverse_ms(&s_state, 50u));
    TEST_ASSERT_TRUE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                            BOARD_GPIO_MOTOR_EN_PIN));
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_PH_PORT,
                                             BOARD_GPIO_MOTOR_PH_PIN));
}

void test_motor_run_rejects_null_state(void)
{
    motor_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      motor_driver_run_forward_ms(NULL, 100u));
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      motor_driver_run_reverse_ms(NULL, 100u));
    TEST_ASSERT_EQUAL(0u, fake_gpio_expander_set_pin_calls());
}

void test_motor_run_rejects_null_expander(void)
{
    motor_hw_t hw = {
        .expander = NULL,
        .delay_ms = test_delay_ms,
    };

    fake_gpio_expander_reset();
    motor_driver_init(&s_state, &hw);

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      motor_driver_run_forward_ms(&s_state, 100u));
    TEST_ASSERT_EQUAL(0u, fake_gpio_expander_set_pin_calls());
}

void test_motor_run_reverse_accepts_max_duration(void)
{
    motor_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_driver_run_reverse_ms(&s_state, MOTOR_RUN_MS_MAX));
    TEST_ASSERT_EQUAL(MOTOR_PH_SETTLE_MS + MOTOR_RUN_MS_MAX,
                      (unsigned)fake_time_ticks());
}

void test_motor_driver_init_null_safe(void)
{
    motor_driver_init(NULL, NULL);
    motor_driver_init(&s_state, NULL);
}

void test_motor_start_forward_leaves_en_high_until_stop(void)
{
    motor_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK, motor_driver_start_forward(&s_state));
    TEST_ASSERT_TRUE(s_state.running);
    TEST_ASSERT_TRUE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                             BOARD_GPIO_MOTOR_EN_PIN));
    TEST_ASSERT_EQUAL(MOTOR_PH_SETTLE_MS, (unsigned)fake_time_ticks());
    TEST_ASSERT_EQUAL(PORT_OK, motor_driver_stop(&s_state));
    TEST_ASSERT_FALSE(s_state.running);
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_EN_PORT,
                                              BOARD_GPIO_MOTOR_EN_PIN));
}

void test_motor_start_reverse_and_is_running(void)
{
    motor_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK, motor_driver_start_reverse(&s_state));
    TEST_ASSERT_TRUE(motor_driver_is_running(&s_state));
    TEST_ASSERT_FALSE(fake_gpio_expander_pin(BOARD_GPIO_MOTOR_PH_PORT,
                                             BOARD_GPIO_MOTOR_PH_PIN));
    (void)motor_driver_stop(&s_state);
}

void test_motor_start_rejects_overlap(void)
{
    motor_driver_test_reset();
    TEST_ASSERT_EQUAL(PORT_OK, motor_driver_start_forward(&s_state));
    TEST_ASSERT_EQUAL(PORT_ERR_BUSY, motor_driver_start_forward(&s_state));
    (void)motor_driver_stop(&s_state);
}
