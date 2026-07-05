/* Tests: spec/30-processes/wfci-bus-arbitration.md */

#include "unity.h"

#include "fake_time.h"
#include "fake_wfci_bus_port.h"
#include "wfci_bus_port.h"

void test_wfci_bus_acquire_release_records_profile(void)
{
    size_t count;
    const fake_wfci_bus_record_t *rec;

    fake_wfci_bus_reset();
    fake_time_reset();

    TEST_ASSERT_EQUAL(PORT_OK,
                      wfci_bus_port_get()->acquire(WFCI_BUS_PROFILE_EXPANDER,
                                                   WFCI_BUS_PRIORITY_HIGH,
                                                   100u));
    fake_time_advance_ms(2u);
    wfci_bus_port_get()->release(WFCI_BUS_PROFILE_EXPANDER);

    rec = fake_wfci_bus_acquires(&count);
    TEST_ASSERT_EQUAL(1u, count);
    TEST_ASSERT_EQUAL(WFCI_BUS_PROFILE_EXPANDER, rec[0].profile);
    TEST_ASSERT_EQUAL(WFCI_BUS_PRIORITY_HIGH, rec[0].priority);
    TEST_ASSERT_EQUAL(1u, fake_wfci_bus_release_count());
    TEST_ASSERT_EQUAL(2u, rec[0].hold_ms);
}

void test_wfci_bus_try_acquire_fails_when_busy(void)
{
    fake_wfci_bus_reset();

    TEST_ASSERT_EQUAL(PORT_OK,
                      wfci_bus_port_get()->acquire(WFCI_BUS_PROFILE_DISPLAY,
                                                   WFCI_BUS_PRIORITY_NORMAL,
                                                   0u));
    TEST_ASSERT_EQUAL(PORT_ERR_BUSY,
                      wfci_bus_port_get()->try_acquire(WFCI_BUS_PROFILE_EXPANDER,
                                                       WFCI_BUS_PRIORITY_HIGH));
    wfci_bus_port_get()->release(WFCI_BUS_PROFILE_DISPLAY);
}

void test_wfci_bus_acquire_waits_for_release(void)
{
    fake_wfci_bus_reset();
    fake_time_reset();

    TEST_ASSERT_EQUAL(PORT_OK,
                      wfci_bus_port_get()->acquire(WFCI_BUS_PROFILE_DISPLAY,
                                                   WFCI_BUS_PRIORITY_NORMAL,
                                                   100u));
    fake_wfci_bus_arm_release_after_ms(3u);
    fake_time_advance_ms(1u);

    TEST_ASSERT_EQUAL(PORT_OK,
                      wfci_bus_port_get()->acquire(WFCI_BUS_PROFILE_EXPANDER,
                                                   WFCI_BUS_PRIORITY_NORMAL,
                                                   100u));
    TEST_ASSERT_EQUAL(1u, fake_wfci_bus_release_count());
    wfci_bus_port_get()->release(WFCI_BUS_PROFILE_EXPANDER);
    TEST_ASSERT_EQUAL(2u, fake_wfci_bus_release_count());
}

void test_wfci_bus_acquire_times_out_when_held_too_long(void)
{
    fake_wfci_bus_reset();
    fake_time_reset();

    TEST_ASSERT_EQUAL(PORT_OK,
                      wfci_bus_port_get()->acquire(WFCI_BUS_PROFILE_DISPLAY,
                                                   WFCI_BUS_PRIORITY_NORMAL,
                                                   0u));
    fake_wfci_bus_arm_release_after_ms(200u);

    TEST_ASSERT_EQUAL(PORT_ERR_BUSY,
                      wfci_bus_port_get()->acquire(WFCI_BUS_PROFILE_EXPANDER,
                                                   WFCI_BUS_PRIORITY_NORMAL,
                                                   50u));
    wfci_bus_port_get()->release(WFCI_BUS_PROFILE_DISPLAY);
}

void test_wfci_bus_hold_duration_tracked(void)
{
    fake_wfci_bus_reset();
    fake_time_reset();

    TEST_ASSERT_EQUAL(PORT_OK,
                      wfci_bus_port_get()->acquire(WFCI_BUS_PROFILE_EXPANDER,
                                                   WFCI_BUS_PRIORITY_NORMAL,
                                                   0u));
    fake_time_advance_ms(7u);
    wfci_bus_port_get()->release(WFCI_BUS_PROFILE_EXPANDER);

    TEST_ASSERT_EQUAL(7u, fake_wfci_bus_max_hold_ms());
}
