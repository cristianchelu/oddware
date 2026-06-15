/* Tests: spec/30-processes/wifi-lifecycle.md */

#include "unity.h"

#include "fake_wifi_port.h"
#include "wifi_session.h"

void test_session_down_calls_disconnect_once(void)
{
    const fake_wifi_port_state_t *wifi;

    fake_wifi_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, wifi_session_down());

    wifi = fake_wifi_port_state();
    TEST_ASSERT_EQUAL_UINT32(1u, wifi->disconnect_calls);
}

void test_session_connect_calls_radio_up_connect_wait_ready(void)
{
    const fake_wifi_port_state_t *wifi;

    fake_wifi_port_reset();
    fake_wifi_port_set_sta_up(false, false);

    TEST_ASSERT_EQUAL(PORT_OK,
                      wifi_session_connect("HomeNet", "secret", 5000u));

    wifi = fake_wifi_port_state();
    TEST_ASSERT_EQUAL_UINT32(1u, wifi->radio_up_calls);
    TEST_ASSERT_EQUAL_UINT32(1u, wifi->connect_calls);
    TEST_ASSERT_EQUAL_UINT32(1u, wifi->wait_ready_calls);
    TEST_ASSERT_TRUE(wifi->connected);
    TEST_ASSERT_TRUE(wifi->has_ip);
}

void test_session_connect_after_down_round_trip(void)
{
    const fake_wifi_port_state_t *wifi;

    fake_wifi_port_reset();
    fake_wifi_port_set_sta_up(true, true);

    TEST_ASSERT_EQUAL(PORT_OK, wifi_session_down());
    TEST_ASSERT_EQUAL(PORT_OK,
                      wifi_session_connect("HomeNet", "secret", 5000u));

    wifi = fake_wifi_port_state();
    TEST_ASSERT_EQUAL_UINT32(1u, wifi->disconnect_calls);
    TEST_ASSERT_EQUAL_UINT32(1u, wifi->radio_up_calls);
    TEST_ASSERT_TRUE(wifi->connected);
    TEST_ASSERT_TRUE(wifi->has_ip);
}
