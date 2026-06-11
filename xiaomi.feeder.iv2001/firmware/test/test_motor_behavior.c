/* Behavior tests: motor index sensing and dispense — not IRQ-scripted wiring checks.
 * spec/30-processes/dispense-cycle.md, spec/30-processes/motor-index.md */

#include "unity.h"

#include "app.h"
#include "cli_test_assert.h"
#include "app_event.h"
#include "app_event_port.h"
#include "dispense_cli.h"
#include "fake_adc_port.h"
#include "fake_gpio_expander_port.h"
#include "fake_motor_index_port.h"
#include "fake_time.h"
#include "fake_wfci_bus_port.h"
#include "motor_ctrl.h"
#include "motor_ctrl_test.h"
#include "motor_index_port_provider_host.h"
#include "motor_jam.h"
#include "motor_port_provider_host.h"
#include "wfci_bus_port.h"

extern void fake_app_event_q_reset(void);
extern void freertos_notify_test_reset(void);

static void motor_behavior_reset_providers(void)
{
    motor_index_port_host_reset();
    motor_port_host_reset();
}

static void motor_behavior_setup_unit(void)
{
    motor_behavior_reset_providers();
    fake_gpio_expander_reset();
    fake_wfci_bus_reset();
    fake_motor_index_port_reset();
    fake_adc_port_reset();
    fake_time_reset();
    freertos_notify_test_reset();
    fake_app_event_q_reset();
    app_event_port_init();
    motor_ctrl_start();
    motor_ctrl_test_reset();
}

static void motor_behavior_setup_adapter_stack(void)
{
    motor_behavior_reset_providers();
    fake_gpio_expander_reset();
    fake_wfci_bus_reset();
    fake_adc_port_reset();
    fake_time_reset();
    freertos_notify_test_reset();
    fake_app_event_q_reset();
    app_event_port_init();
    motor_index_port_host_use_adapter(true);
    motor_port_host_use_integration(true);
    motor_ctrl_start();
    motor_ctrl_test_reset();
    fake_gpio_expander_set_index_beam_open(false);
}

static void motor_behavior_run_motor_until_idle(uint32_t max_polls)
{
    uint32_t i;

    for (i = 0u; i < max_polls; i++) {
        motor_ctrl_test_poll();
        if (!motor_ctrl_is_active()) {
            break;
        }
    }
}

static bool motor_behavior_take_event(app_event_type_t expect)
{
    app_event_t ev;

    if (!app_event_try_receive(&ev)) {
        return false;
    }

    TEST_ASSERT_EQUAL(expect, ev.type);
    app_event_release(&ev);
    return true;
}

static void motor_behavior_simulate_index_hole(void)
{
    fake_gpio_expander_set_index_beam_open(true);
}

void test_motor_burst_completes_on_index_transition_without_irq(void)
{
    motor_behavior_setup_unit();
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_ctrl_request_burst(1u, MOTOR_BURST_TIMEOUT_MS));

    motor_ctrl_test_poll();
    TEST_ASSERT_TRUE(motor_ctrl_is_active());

    fake_motor_index_port_set_beam_open(true);
    fake_time_advance_ms(MOTOR_BURST_BEAM_POLL_MS + 1u);
    motor_behavior_run_motor_until_idle(50u);

    TEST_ASSERT_FALSE(motor_ctrl_is_active());
    TEST_ASSERT_TRUE(motor_behavior_take_event(EVT_BURST_DONE));
}

void test_motor_burst_survives_wfci_busy_then_counts_pulse(void)
{
    motor_behavior_setup_adapter_stack();
    TEST_ASSERT_EQUAL(PORT_OK,
                      wfci_bus_port_get()->acquire(WFCI_BUS_PROFILE_ADC,
                                                   WFCI_BUS_PRIORITY_NORMAL,
                                                   0u));
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_ctrl_request_burst(1u, MOTOR_BURST_TIMEOUT_MS));

    motor_ctrl_test_poll();
    TEST_ASSERT_TRUE(motor_ctrl_is_active());

    motor_behavior_simulate_index_hole();
    fake_time_advance_ms(MOTOR_BURST_TIMEOUT_MS / 4u);
    motor_ctrl_test_poll();
    TEST_ASSERT_TRUE(motor_ctrl_is_active());

    wfci_bus_port_get()->release(WFCI_BUS_PROFILE_ADC);
    fake_time_advance_ms(MOTOR_BURST_BEAM_POLL_MS + 1u);
    motor_behavior_run_motor_until_idle(50u);

    TEST_ASSERT_FALSE(motor_ctrl_is_active());
    TEST_ASSERT_TRUE(motor_behavior_take_event(EVT_BURST_DONE));
}

void test_motor_burst_retries_index_sample_after_poll_busy(void)
{
    motor_behavior_setup_unit();
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_ctrl_request_burst(1u, MOTOR_BURST_TIMEOUT_MS));

    motor_ctrl_test_poll();
    fake_motor_index_port_set_poll_busy_remaining(2u);
    motor_ctrl_test_notify(MOTOR_CTRL_NOTIFY_INDEX);
    fake_time_advance_ms(MOTOR_INDEX_IRQ_DEBOUNCE_MS + 1u);
    fake_motor_index_port_set_beam_open(true);
    motor_behavior_run_motor_until_idle(50u);

    TEST_ASSERT_FALSE(motor_ctrl_is_active());
    TEST_ASSERT_TRUE(motor_behavior_take_event(EVT_BURST_DONE));
}

void test_dispense_completes_when_index_pulse_seen_e2e(void)
{
    motor_behavior_setup_adapter_stack();
    dispense_cli_test_reset();
    app_test_reset();

    cli_test_reset();
    (void)dispense_cli_handle(0u, NULL);
    assert_cli_body("dispense started");
    TEST_ASSERT_TRUE(app_step());
    motor_ctrl_test_poll();
    TEST_ASSERT_TRUE(motor_ctrl_is_active());

    motor_behavior_simulate_index_hole();
    fake_time_advance_ms(MOTOR_BURST_BEAM_POLL_MS + 1u);
    motor_behavior_run_motor_until_idle(100u);
    TEST_ASSERT_FALSE(motor_ctrl_is_active());

    cli_test_reset();
    TEST_ASSERT_TRUE(app_step());
    assert_cli_body("dispense done");

    {
        app_event_t ev;

        TEST_ASSERT_FALSE(app_event_try_receive(&ev));
    }
}

void test_dispense_does_not_park_after_burst_pulse(void)
{
    motor_behavior_setup_adapter_stack();
    dispense_cli_test_reset();
    app_test_reset();

    (void)dispense_cli_handle(0u, NULL);
    (void)app_step();
    motor_ctrl_test_poll();

    motor_behavior_simulate_index_hole();
    fake_time_advance_ms(MOTOR_BURST_BEAM_POLL_MS + 1u);
    motor_behavior_run_motor_until_idle(100u);

    TEST_ASSERT_FALSE(motor_ctrl_is_active());
    TEST_ASSERT_TRUE(motor_behavior_take_event(EVT_BURST_DONE));

    {
        app_event_t ev;

        TEST_ASSERT_FALSE(app_event_try_receive(&ev));
    }
}
