#ifndef REMOTE_CLI_H
#define REMOTE_CLI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

void remote_cli_start(void);
void remote_cli_poll_override(void);
void remote_cli_request_disconnect(void);
void remote_cli_suspend_for_ota(void);
bool remote_cli_wait_suspended_for_ota(uint32_t timeout_ms);
void remote_cli_resume_after_ota(void);

#ifdef HOST_TEST
void remote_cli_test_reset(void);
int remote_cli_test_session_end_get_byte(void);
bool remote_cli_test_begin_session(void);
void remote_cli_test_end_session(void);
void remote_cli_test_peer_hangup(void);
void remote_cli_test_set_reconnect_pending(bool pending);
void remote_cli_test_session_io_tick(void);
bool remote_cli_test_service_session(void);
bool remote_cli_test_sink_attached(void);
bool remote_cli_test_session_active(void);
const char *remote_cli_test_sink_text(void);
int remote_cli_test_normalize_input(int c);
int remote_cli_test_telnet_feed(int c);
const unsigned char *remote_cli_test_telnet_tx(void);
size_t remote_cli_test_telnet_tx_len(void);
bool remote_cli_test_is_suspended_for_ota(void);
#endif

#endif /* REMOTE_CLI_H */
