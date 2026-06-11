/* Tests: spec/30-processes/jam-detection.md */

#include "unity.h"

#include "app_event.h"
#include "app_event_port.h"
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

extern void fake_app_event_q_reset(void);
extern void freertos_notify_test_reset(void);

static void motor_jam_test_setup(void)
{
    motor_index_port_host_reset();
    motor_port_host_reset();
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

static void motor_jam_run_until_fault(uint32_t max_polls)
{
    uint32_t i;

    for (i = 0u; i < max_polls; i++) {
        motor_ctrl_test_poll();
        if (!motor_ctrl_is_active()) {
            return;
        }
    }
}

static bool motor_jam_got_fault(void)
{
    app_event_t ev;

    if (!app_event_try_receive(&ev)) {
        return false;
    }

    TEST_ASSERT_EQUAL(EVT_MOTOR_FAULT, ev.type);
    app_event_release(&ev);
    return true;
}

void test_motor_jam_instant_adc_notify_stops_motor(void)
{
    motor_jam_test_setup();
    fake_adc_port_set_motor_ma(MOTOR_JAM_SUSTAINED_MA + 100u);
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_ctrl_request_burst(1u, MOTOR_BURST_TIMEOUT_MS));
    motor_ctrl_test_poll();
    motor_ctrl_test_notify(MOTOR_CTRL_NOTIFY_ADC_JAM);
    motor_jam_run_until_fault(2000u);
    TEST_ASSERT_TRUE(motor_jam_got_fault());
}

void test_motor_jam_sustained_load_triggers_fault(void)
{
    motor_jam_test_setup();
    fake_adc_port_set_motor_ma(MOTOR_JAM_SUSTAINED_MA + 50u);
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_ctrl_request_burst(1u, MOTOR_BURST_TIMEOUT_MS));
    motor_ctrl_test_poll();
    motor_jam_run_until_fault(2000u);
    TEST_ASSERT_TRUE(motor_jam_got_fault());
}

void test_motor_jam_index_timeout_without_pulses(void)
{
    motor_jam_test_setup();
    TEST_ASSERT_EQUAL(PORT_OK,
                      motor_ctrl_request_burst(1u, MOTOR_BURST_TIMEOUT_MS));
    motor_ctrl_test_poll();
    fake_time_advance_ms(MOTOR_BURST_TIMEOUT_MS + MOTOR_CTRL_LOOP_SLICE_MS);
    motor_jam_run_until_fault(5000u);
    TEST_ASSERT_TRUE(motor_jam_got_fault());
}
