#ifndef REMOTE_CLI_H
#define REMOTE_CLI_H

#include <stdbool.h>
#include <stddef.h>

void remote_cli_start(void);
void remote_cli_poll_override(void);
void remote_cli_request_disconnect(void);

#ifdef HOST_TEST
void remote_cli_test_reset(void);
bool remote_cli_test_begin_session(void);
void remote_cli_test_end_session(void);
bool remote_cli_test_service_session(void);
bool remote_cli_test_sink_attached(void);
bool remote_cli_test_session_active(void);
const char *remote_cli_test_sink_text(void);
int remote_cli_test_normalize_input(int c);
int remote_cli_test_telnet_feed(int c);
const unsigned char *remote_cli_test_telnet_tx(void);
size_t remote_cli_test_telnet_tx_len(void);
#endif

#endif /* REMOTE_CLI_H */
