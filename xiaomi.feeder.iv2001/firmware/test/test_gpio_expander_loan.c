/* Tests: spec/30-processes/wfci-bus-arbitration.md § GPIO expander micro-session */

#include "unity.h"

#include "fake_wfci_bus_port.h"
#include "gpio_expander_loan.h"
#include "wfci_bus_port.h"
#include "wfci_bus_state.h"

void test_gpio_expander_loan_acquires_expander_profile(void)
{
    size_t count;
    const fake_wfci_bus_record_t *rec;

    fake_wfci_bus_reset();
    TEST_ASSERT_EQUAL(PORT_OK, gpio_expander_loan_begin());
    TEST_ASSERT_TRUE(gpio_expander_loan_is_held());
    gpio_expander_loan_end();

    rec = fake_wfci_bus_acquires(&count);
    TEST_ASSERT_EQUAL(1u, count);
    TEST_ASSERT_EQUAL(WFCI_BUS_PROFILE_EXPANDER, rec[0].profile);
    TEST_ASSERT_EQUAL(1u, fake_wfci_bus_release_count());
}

void test_gpio_expander_loan_nested_under_display(void)
{
    size_t count;

    fake_wfci_bus_reset();
    TEST_ASSERT_EQUAL(PORT_OK,
                      wfci_bus_port_get()->acquire(WFCI_BUS_PROFILE_DISPLAY,
                                                   WFCI_BUS_PRIORITY_NORMAL,
                                                   0u));
    TEST_ASSERT_TRUE(wfci_bus_expander_accessible());

    TEST_ASSERT_EQUAL(PORT_OK, gpio_expander_loan_begin());
    gpio_expander_loan_end();
    wfci_bus_port_get()->release(WFCI_BUS_PROFILE_DISPLAY);

    fake_wfci_bus_acquires(&count);
    TEST_ASSERT_EQUAL(1u, count);
    TEST_ASSERT_EQUAL(1u, fake_wfci_bus_release_count());
}

void test_gpio_expander_loan_rejects_double_begin(void)
{
    fake_wfci_bus_reset();
    TEST_ASSERT_EQUAL(PORT_OK, gpio_expander_loan_begin());
    TEST_ASSERT_EQUAL(PORT_ERR_BUSY, gpio_expander_loan_begin());
    gpio_expander_loan_end();
}
