/* Tests: spec/30-processes/uart-console.md § Remote telnet console */

#include <string.h>

#include "unity.h"

#include "app_log.h"
#include "console_mux.h"
#include "console_uart.h"
#include "fake_time.h"
#include "remote_cli.h"

static void remote_cli_test_reset_all(void)
{
    fake_time_reset();
    app_log_test_reset();
    console_mux_test_reset();
    console_uart_test_reset();
    remote_cli_test_reset();
}

void test_remote_cli_session_attaches_log_sink(void)
{
    remote_cli_test_reset_all();

    TEST_ASSERT_TRUE(remote_cli_test_begin_session());
    TEST_ASSERT_TRUE(remote_cli_test_sink_attached());
    TEST_ASSERT_TRUE(remote_cli_test_session_active());

    app_log_info("cli", "host saved");

    remote_cli_test_end_session();

    TEST_ASSERT_FALSE(remote_cli_test_sink_attached());
    TEST_ASSERT_FALSE(remote_cli_test_session_active());
}

void test_remote_cli_rejects_second_session(void)
{
    remote_cli_test_reset_all();

    TEST_ASSERT_TRUE(remote_cli_test_begin_session());
    TEST_ASSERT_FALSE(remote_cli_test_begin_session());
}

void test_remote_cli_mux_released_after_disconnect(void)
{
    remote_cli_test_reset_all();

    TEST_ASSERT_TRUE(remote_cli_test_begin_session());
    remote_cli_test_end_session();

    TEST_ASSERT_FALSE(console_mux_remote_active());
    TEST_ASSERT_TRUE(console_mux_try_remote());
    console_mux_release_remote();
}

void test_remote_cli_disconnect_no_local_message(void)
{
    remote_cli_test_reset_all();

    TEST_ASSERT_TRUE(remote_cli_test_begin_session());
    app_log_info("wifi", "still connected");

    remote_cli_test_end_session();

    TEST_ASSERT_FALSE(remote_cli_test_session_active());
    TEST_ASSERT_NULL(strstr(app_log_test_last_line(), "[console] local"));
}

void test_remote_cli_log_mirror_while_session_active(void)
{
    remote_cli_test_reset_all();

    TEST_ASSERT_TRUE(remote_cli_test_begin_session());

    app_log_info("wifi", "STA ready, IP 192.168.1.10");

    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "[wifi]"));
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "STA ready"));
    TEST_ASSERT_NOT_NULL(strstr(remote_cli_test_sink_text(), "[wifi]"));
    TEST_ASSERT_NOT_NULL(strstr(remote_cli_test_sink_text(), "STA ready"));

    remote_cli_test_end_session();
}

void test_remote_cli_normalize_lf_to_cr(void)
{
    TEST_ASSERT_EQUAL_INT('\r', remote_cli_test_normalize_input('\n'));
    TEST_ASSERT_EQUAL_INT('d', remote_cli_test_normalize_input('d'));
}

void test_remote_cli_telnet_plain_byte_passes_through(void)
{
    remote_cli_test_reset();

    TEST_ASSERT_EQUAL_INT('h', remote_cli_test_telnet_feed('h'));
    TEST_ASSERT_EQUAL_INT('\r', remote_cli_test_telnet_feed('\n'));
    TEST_ASSERT_EQUAL_UINT(0, remote_cli_test_telnet_tx_len());
}

void test_remote_cli_telnet_do_echo_replies_wont(void)
{
    remote_cli_test_reset();

    TEST_ASSERT_EQUAL_INT(-1, remote_cli_test_telnet_feed(255));
    TEST_ASSERT_EQUAL_INT(-1, remote_cli_test_telnet_feed(253)); /* DO */
    TEST_ASSERT_EQUAL_INT(-1, remote_cli_test_telnet_feed(1));   /* ECHO */

    TEST_ASSERT_EQUAL_UINT(3, remote_cli_test_telnet_tx_len());
    TEST_ASSERT_EQUAL_UINT8(255, remote_cli_test_telnet_tx()[0]);
    TEST_ASSERT_EQUAL_UINT8(252, remote_cli_test_telnet_tx()[1]); /* WONT */
    TEST_ASSERT_EQUAL_UINT8(1, remote_cli_test_telnet_tx()[2]);
}

void test_remote_cli_telnet_will_sga_replies_dont(void)
{
    remote_cli_test_reset();

    TEST_ASSERT_EQUAL_INT(-1, remote_cli_test_telnet_feed(255));
    TEST_ASSERT_EQUAL_INT(-1, remote_cli_test_telnet_feed(251)); /* WILL */
    TEST_ASSERT_EQUAL_INT(-1, remote_cli_test_telnet_feed(3));   /* SGA */

    TEST_ASSERT_EQUAL_UINT(3, remote_cli_test_telnet_tx_len());
    TEST_ASSERT_EQUAL_UINT8(255, remote_cli_test_telnet_tx()[0]);
    TEST_ASSERT_EQUAL_UINT8(254, remote_cli_test_telnet_tx()[1]); /* DONT */
    TEST_ASSERT_EQUAL_UINT8(3, remote_cli_test_telnet_tx()[2]);
}

void test_remote_cli_telnet_client_burst_then_command(void)
{
    const unsigned char burst[] = {
        255, 253, 1,   /* DO ECHO */
        255, 253, 3,   /* DO SGA */
        255, 251, 3,   /* WILL SGA */
    };
    size_t i;

    remote_cli_test_reset();

    for (i = 0; i < sizeof(burst); i++) {
        TEST_ASSERT_EQUAL_INT(-1, remote_cli_test_telnet_feed((int)burst[i]));
    }

    TEST_ASSERT_EQUAL_UINT(9, remote_cli_test_telnet_tx_len());
    TEST_ASSERT_EQUAL_INT('b', remote_cli_test_telnet_feed('b'));
    TEST_ASSERT_EQUAL_INT('a', remote_cli_test_telnet_feed('a'));
    TEST_ASSERT_EQUAL_INT('\r', remote_cli_test_telnet_feed('\r'));
}

void test_remote_cli_telnet_escaped_iac_delivers_ff(void)
{
    remote_cli_test_reset();

    TEST_ASSERT_EQUAL_INT(-1, remote_cli_test_telnet_feed(255));
    TEST_ASSERT_EQUAL_INT(255, remote_cli_test_telnet_feed(255));
}

void test_remote_cli_telnet_subnegotiation_skipped(void)
{
    remote_cli_test_reset();

    TEST_ASSERT_EQUAL_INT(-1, remote_cli_test_telnet_feed(255));
    TEST_ASSERT_EQUAL_INT(-1, remote_cli_test_telnet_feed(250)); /* SB */
    TEST_ASSERT_EQUAL_INT(-1, remote_cli_test_telnet_feed(0));
    TEST_ASSERT_EQUAL_INT(-1, remote_cli_test_telnet_feed(255));
    TEST_ASSERT_EQUAL_INT(-1, remote_cli_test_telnet_feed(240));   /* SE */
    TEST_ASSERT_EQUAL_INT('z', remote_cli_test_telnet_feed('z'));
}

void test_remote_cli_uart_override_via_poll_and_service(void)
{
    remote_cli_test_reset_all();

    TEST_ASSERT_TRUE(remote_cli_test_begin_session());
    console_uart_test_inject((uint8_t)'\r');

    remote_cli_poll_override();

    TEST_ASSERT_FALSE(console_mux_force_local_pending());
    TEST_ASSERT_FALSE(remote_cli_test_session_active());
    TEST_ASSERT_FALSE(console_mux_remote_active());
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "[console] local"));
}

void test_remote_cli_request_disconnect_ends_session(void)
{
    remote_cli_test_reset_all();

    TEST_ASSERT_TRUE(remote_cli_test_begin_session());
    remote_cli_request_disconnect();

    TEST_ASSERT_FALSE(remote_cli_test_session_active());
    TEST_ASSERT_FALSE(console_mux_remote_active());
    TEST_ASSERT_TRUE(console_mux_try_remote());
    console_mux_release_remote();
}

void test_remote_cli_request_disconnect_no_local_message(void)
{
    remote_cli_test_reset_all();

    TEST_ASSERT_TRUE(remote_cli_test_begin_session());
    app_log_info("cli", "before exit");
    remote_cli_request_disconnect();

    TEST_ASSERT_NULL(strstr(app_log_test_last_line(), "[console] local"));
}

void test_remote_cli_force_local_service_matches_device_path(void)
{
    remote_cli_test_reset_all();

    TEST_ASSERT_TRUE(remote_cli_test_begin_session());
    console_mux_request_force_local();

    TEST_ASSERT_TRUE(console_mux_force_local_pending());
    TEST_ASSERT_FALSE(remote_cli_test_service_session());

    TEST_ASSERT_FALSE(remote_cli_test_session_active());
    TEST_ASSERT_FALSE(console_mux_remote_active());
    TEST_ASSERT_NOT_NULL(strstr(app_log_test_last_line(), "[console] local"));
}

void test_remote_cli_suspend_for_ota_releases_mux(void)
{
    remote_cli_test_reset_all();

    TEST_ASSERT_TRUE(remote_cli_test_begin_session());
    remote_cli_suspend_for_ota();

    TEST_ASSERT_FALSE(remote_cli_test_session_active());
    TEST_ASSERT_FALSE(console_mux_remote_active());
    TEST_ASSERT_TRUE(remote_cli_test_is_suspended_for_ota());
}

void test_remote_cli_suspend_rejects_new_session(void)
{
    remote_cli_test_reset_all();

    remote_cli_suspend_for_ota();

    TEST_ASSERT_FALSE(remote_cli_test_begin_session());
}

void test_remote_cli_resume_after_ota_allows_session(void)
{
    remote_cli_test_reset_all();

    remote_cli_suspend_for_ota();
    remote_cli_resume_after_ota();

    TEST_ASSERT_FALSE(remote_cli_test_is_suspended_for_ota());
    TEST_ASSERT_TRUE(remote_cli_test_begin_session());
    remote_cli_test_end_session();
}
