/*
 * Remote telnet CLI — spec/30-processes/uart-console.md § Remote telnet console
 */

#include "remote_cli.h"

#include <string.h>

#include "app_log.h"
#include "console_mux.h"
#include "console_uart.h"

#if REMOTE_CLI_ENABLE

#define TELNET_IAC   255
#define TELNET_SE    240
#define TELNET_SB    250
#define TELNET_WILL  251
#define TELNET_WONT  252
#define TELNET_DO    253
#define TELNET_DONT  254

typedef enum {
    REMOTE_CLI_TN_DATA,
    REMOTE_CLI_TN_IAC,
    REMOTE_CLI_TN_OPT,
    REMOTE_CLI_TN_SB,
    REMOTE_CLI_TN_SB_IAC,
} remote_cli_telnet_state_t;

static remote_cli_telnet_state_t s_tn_state;
static uint8_t s_tn_iac_cmd;

static int remote_cli_normalize_input(int c)
{
    if (c == '\n') {
        return '\r';
    }

    return c;
}

static void remote_cli_telnet_reset(void)
{
    s_tn_state = REMOTE_CLI_TN_DATA;
    s_tn_iac_cmd = 0;
}

#ifdef HOST_TEST
static uint8_t s_host_tn_tx[64];
static size_t s_host_tn_tx_len;

static void remote_cli_telnet_send_neg(uint8_t cmd, uint8_t opt)
{
    if (s_host_tn_tx_len + 3u > sizeof(s_host_tn_tx)) {
        return;
    }

    s_host_tn_tx[s_host_tn_tx_len++] = TELNET_IAC;
    s_host_tn_tx[s_host_tn_tx_len++] = cmd;
    s_host_tn_tx[s_host_tn_tx_len++] = opt;
}
#else
static void remote_cli_telnet_send_neg(uint8_t cmd, uint8_t opt);
#endif

/* Returns CLI byte (>=0) or -1 when consumed by telnet framing. */
static int remote_cli_telnet_rx_byte(uint8_t c)
{
    switch (s_tn_state) {
    case REMOTE_CLI_TN_DATA:
        if (c == TELNET_IAC) {
            s_tn_state = REMOTE_CLI_TN_IAC;
            return -1;
        }
        return remote_cli_normalize_input((int)c);

    case REMOTE_CLI_TN_IAC:
        if (c == TELNET_IAC) {
            s_tn_state = REMOTE_CLI_TN_DATA;
            return remote_cli_normalize_input((int)TELNET_IAC);
        }
        if (c == TELNET_WILL || c == TELNET_WONT || c == TELNET_DO || c == TELNET_DONT) {
            s_tn_iac_cmd = c;
            s_tn_state = REMOTE_CLI_TN_OPT;
            return -1;
        }
        if (c == TELNET_SB) {
            s_tn_state = REMOTE_CLI_TN_SB;
            return -1;
        }
        s_tn_state = REMOTE_CLI_TN_DATA;
        return -1;

    case REMOTE_CLI_TN_OPT:
        if (s_tn_iac_cmd == TELNET_DO) {
            remote_cli_telnet_send_neg(TELNET_WONT, c);
        } else if (s_tn_iac_cmd == TELNET_WILL) {
            remote_cli_telnet_send_neg(TELNET_DONT, c);
        }
        s_tn_state = REMOTE_CLI_TN_DATA;
        return -1;

    case REMOTE_CLI_TN_SB:
        if (c == TELNET_IAC) {
            s_tn_state = REMOTE_CLI_TN_SB_IAC;
        }
        return -1;

    case REMOTE_CLI_TN_SB_IAC:
        if (c == TELNET_SE) {
            s_tn_state = REMOTE_CLI_TN_DATA;
        } else if (c == TELNET_IAC) {
            s_tn_state = REMOTE_CLI_TN_SB;
        } else {
            s_tn_state = REMOTE_CLI_TN_SB;
        }
        return -1;

    default:
        remote_cli_telnet_reset();
        return -1;
    }
}

#ifndef HOST_TEST

#include <errno.h>
#include <fcntl.h>

#include "FreeRTOS.h"
#include "task.h"

#include "cli.h"
#include "lwip/sockets.h"
#include "task_def.h"

#include "app_cli.h"

#define REMOTE_CLI_PORT           2323
#define REMOTE_CLI_STACK_WORDS    (APP_TASK_STACKSIZE / sizeof(portSTACK_TYPE))

static TaskHandle_t s_remote_cli_task;
static int s_listen_fd = -1;
static int s_client_fd = -1;
static volatile bool s_session_end;
static volatile bool s_suspended_for_ota;
static volatile bool s_remote_cli_reclaimed;
static cli_t s_remote_cli;

static void remote_cli_detach_session(void);
static void remote_cli_close_listener(void);
static void remote_cli_enter_ota_suspend(void);

static void remote_cli_telnet_send_neg(uint8_t cmd, uint8_t opt)
{
    uint8_t buf[3];
    ssize_t sent;

    if (s_client_fd < 0) {
        return;
    }

    buf[0] = TELNET_IAC;
    buf[1] = cmd;
    buf[2] = opt;
    sent = send(s_client_fd, buf, 3, 0);
    if (sent != 3) {
        s_session_end = true;
    }
}

static void remote_cli_detach_log_sink(void)
{
    app_log_clear_sink();
}

static void remote_cli_log_sink(const char *buf, size_t len, void *ctx)
{
    int fd;
    ssize_t sent;

    (void)ctx;

    fd = s_client_fd;
    if (fd < 0 || buf == NULL || len == 0u) {
        return;
    }

    sent = send(fd, buf, len, 0);
    if (sent < 0 || (size_t)sent != len) {
        s_session_end = true;
    }
}

static int remote_cli_sock_get(void)
{
    char c;
    int n;

    for (;;) {
        if (console_mux_force_local_pending() || s_session_end) {
            return -1;
        }

        if (s_client_fd < 0) {
            return -1;
        }

        n = recv(s_client_fd, &c, 1, 0);
        if (n == 1) {
            int out = remote_cli_telnet_rx_byte((uint8_t)(unsigned char)c);

            if (out >= 0) {
                return out;
            }
            continue;
        }
        if (n == 0) {
            s_session_end = true;
            return -1;
        }
        if (n < 0 && (errno == EWOULDBLOCK || errno == EAGAIN)) {
            vTaskDelay(1);
            continue;
        }

        s_session_end = true;
        return -1;
    }
}

static int remote_cli_sock_put(int c)
{
    char ch = (char)c;
    ssize_t sent;

    if (s_client_fd < 0) {
        return 0;
    }

    sent = send(s_client_fd, &ch, 1, 0);
    if (sent != 1) {
        s_session_end = true;
        return 0;
    }

    return c;
}

static void remote_cli_detach_session(void)
{
    int fd;

    remote_cli_detach_log_sink();

    fd = s_client_fd;
    if (fd >= 0) {
        s_client_fd = -1;
        (void)shutdown(fd, SHUT_RDWR);
        (void)close(fd);
    }

    if (console_mux_remote_active()) {
        console_mux_release_remote();
    }
}

static void remote_cli_end_session(bool print_local)
{
    remote_cli_detach_session();
    s_session_end = false;

    if (print_local) {
        app_log_info("cli", "[console] local");
    }
}

static bool remote_cli_begin_session(int client_fd)
{
    int flags;

    if (!console_mux_try_remote()) {
        return false;
    }

    flags = fcntl(client_fd, F_GETFL, 0);
    if (flags >= 0) {
        (void)fcntl(client_fd, F_SETFL, flags | O_NONBLOCK);
    }

    s_client_fd = client_fd;
    s_session_end = false;
    remote_cli_telnet_reset();

    app_cli_session_init(&s_remote_cli, remote_cli_sock_get, remote_cli_sock_put);
    app_log_set_sink(remote_cli_log_sink, NULL);
    return true;
}

static void remote_cli_run_session(void)
{
    bool forced = false;

    while (s_client_fd >= 0 && console_mux_remote_active() && !s_session_end && !s_suspended_for_ota) {
        if (console_mux_take_force_local()) {
            forced = true;
            break;
        }
        cli_task();
        if (s_session_end) {
            break;
        }
    }

    remote_cli_end_session(forced);
}

static void remote_cli_close_listener(void)
{
    if (s_listen_fd >= 0) {
        (void)close(s_listen_fd);
        s_listen_fd = -1;
    }
}

static void remote_cli_recover_stale_state(void)
{
    s_session_end = true;
    app_log_clear_sink();

    if (console_mux_remote_active()) {
        console_mux_release_remote();
    }
}

static void remote_cli_enter_ota_suspend(void)
{
    remote_cli_detach_session();
    remote_cli_close_listener();
    s_remote_cli_reclaimed = true;
    s_remote_cli_task = NULL;
    vTaskDelete(NULL);
}

static bool remote_cli_listen_readable(uint32_t timeout_ms)
{
    fd_set readfds;
    struct timeval tv;
    int rc;

    if (s_listen_fd < 0) {
        return false;
    }

    FD_ZERO(&readfds);
    FD_SET(s_listen_fd, &readfds);
    tv.tv_sec = (time_t)(timeout_ms / 1000u);
    tv.tv_usec = (suseconds_t)((timeout_ms % 1000u) * 1000u);

    rc = select(s_listen_fd + 1, &readfds, NULL, NULL, &tv);
    return rc > 0 && FD_ISSET(s_listen_fd, &readfds);
}

static bool remote_cli_open_listener(void)
{
    struct sockaddr_in addr;
    int yes = 1;

    remote_cli_close_listener();

    s_listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (s_listen_fd < 0) {
        app_log_error("cli", "remote console: socket failed (%d)", errno);
        return false;
    }

    (void)setsockopt(s_listen_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = htons(REMOTE_CLI_PORT);

    if (bind(s_listen_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        app_log_error("cli", "remote console: bind port %u failed (%d)",
                      (unsigned)REMOTE_CLI_PORT, errno);
        remote_cli_close_listener();
        return false;
    }

    if (listen(s_listen_fd, 1) < 0) {
        app_log_error("cli", "remote console: listen failed (%d)", errno);
        remote_cli_close_listener();
        return false;
    }

    {
        int flags = fcntl(s_listen_fd, F_GETFL, 0);

        if (flags >= 0) {
            (void)fcntl(s_listen_fd, F_SETFL, flags | O_NONBLOCK);
        }
    }

    return true;
}

static void remote_cli_task(void *param)
{
    (void)param;

    /* Started from EVT_WIFI_STA_READY after wifi_sta already called lwip_net_ready().
     * Do not call lwip_net_ready() here — its semaphores are one-shot and already taken. */

    for (;;) {
        if (s_suspended_for_ota) {
            remote_cli_enter_ota_suspend();
        }

        if (!remote_cli_open_listener()) {
            TickType_t retry_until = xTaskGetTickCount() + pdMS_TO_TICKS(2000);

            while (xTaskGetTickCount() < retry_until) {
                if (s_suspended_for_ota) {
                    break;
                }
                vTaskDelay(pdMS_TO_TICKS(100));
            }
            continue;
        }

        app_log_info("cli", "remote console listening on port %u", (unsigned)REMOTE_CLI_PORT);

        for (;;) {
            struct sockaddr_in peer;
            socklen_t peer_len = sizeof(peer);
            int client_fd;

            if (s_suspended_for_ota) {
                break;
            }

            if (!remote_cli_listen_readable(100u)) {
                continue;
            }

            client_fd = accept(s_listen_fd, (struct sockaddr *)&peer, &peer_len);
            if (client_fd < 0) {
                if (s_suspended_for_ota) {
                    break;
                }
                continue;
            }

            if (console_mux_remote_active()) {
                (void)close(client_fd);
                continue;
            }

            if (!remote_cli_begin_session(client_fd)) {
                (void)close(client_fd);
                continue;
            }

            remote_cli_run_session();
        }
    }
}

void remote_cli_start(void)
{
    if (s_remote_cli_task != NULL) {
        return;
    }

    s_remote_cli_reclaimed = false;

    if (xTaskCreate(remote_cli_task,
                    "remote_cli",
                    REMOTE_CLI_STACK_WORDS,
                    NULL,
                    APP_TASK_PRIO,
                    &s_remote_cli_task) != pdPASS) {
        app_log_error("cli", "remote console: task create failed");
        return;
    }
}

void remote_cli_suspend_for_ota(void)
{
    TaskHandle_t task;

    s_suspended_for_ota = true;
    s_session_end = true;
    app_log_info("cli", "remote console suspended for ota");

    if (remote_cli_wait_suspended_for_ota(2000u)) {
        return;
    }

    task = s_remote_cli_task;
    if (task != NULL) {
        app_log_warn("cli", "remote console: force delete for ota");
        s_remote_cli_task = NULL;
        s_remote_cli_reclaimed = true;
        vTaskDelete(task);
    }
}

bool remote_cli_wait_suspended_for_ota(uint32_t timeout_ms)
{
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

    while (xTaskGetTickCount() < deadline) {
        if (s_remote_cli_reclaimed) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return s_remote_cli_reclaimed;
}

void remote_cli_resume_after_ota(void)
{
    s_suspended_for_ota = false;
    remote_cli_recover_stale_state();

    if (s_remote_cli_task == NULL) {
        remote_cli_start();
    }

    app_log_info("cli", "remote console resumed after ota");
}

void remote_cli_poll_override(void)
{
    if (!console_mux_remote_active()) {
        return;
    }

    if (!console_uart_rx_pending()) {
        return;
    }

    console_uart_consume_pending();
    s_session_end = true;
    app_log_clear_sink();
    console_mux_release_remote();
    app_log_info("cli", "[console] local");
}

void remote_cli_request_disconnect(void)
{
    if (!console_mux_remote_active()) {
        return;
    }

    s_session_end = true;
    remote_cli_detach_session();
}

#else /* HOST_TEST */

static bool s_session_active;
static bool s_sink_attached;
static volatile bool s_suspended_for_ota;
static volatile bool s_remote_cli_reclaimed;
static char s_host_sink[512];
static size_t s_host_sink_len;

static void remote_cli_test_sink(const char *buf, size_t len, void *ctx)
{
    size_t room;

    (void)ctx;

    if (buf == NULL || len == 0u) {
        return;
    }

    room = sizeof(s_host_sink) - s_host_sink_len;
    if (len > room) {
        len = room;
    }

    memcpy(s_host_sink + s_host_sink_len, buf, len);
    s_host_sink_len += len;
}

static void remote_cli_host_end_session(bool print_local)
{
    if (!s_session_active) {
        return;
    }

    app_log_clear_sink();
    s_sink_attached = false;
    console_mux_release_remote();
    s_session_active = false;

    if (print_local) {
        app_log_info("cli", "[console] local");
    }
}

void remote_cli_test_reset(void)
{
    remote_cli_host_end_session(false);
    s_suspended_for_ota = false;
    s_remote_cli_reclaimed = false;
    s_host_sink_len = 0;
    memset(s_host_sink, 0, sizeof(s_host_sink));
    s_host_tn_tx_len = 0;
    memset(s_host_tn_tx, 0, sizeof(s_host_tn_tx));
    remote_cli_telnet_reset();
}

bool remote_cli_test_is_suspended_for_ota(void)
{
    return s_suspended_for_ota;
}

bool remote_cli_test_begin_session(void)
{
    if (s_suspended_for_ota) {
        return false;
    }

    if (!console_mux_try_remote()) {
        return false;
    }

    app_log_set_sink(remote_cli_test_sink, NULL);
    s_sink_attached = true;
    s_session_active = true;
    return true;
}

void remote_cli_test_end_session(void)
{
    remote_cli_host_end_session(false);
}

bool remote_cli_test_service_session(void)
{
    if (!s_session_active) {
        return false;
    }

    if (console_mux_take_force_local()) {
        remote_cli_host_end_session(true);
        return false;
    }

    return true;
}

bool remote_cli_test_sink_attached(void)
{
    return s_sink_attached;
}

bool remote_cli_test_session_active(void)
{
    return s_session_active;
}

const char *remote_cli_test_sink_text(void)
{
    return s_host_sink;
}

int remote_cli_test_normalize_input(int c)
{
    return remote_cli_normalize_input(c);
}

int remote_cli_test_telnet_feed(int c)
{
    return remote_cli_telnet_rx_byte((uint8_t)(unsigned char)c);
}

const unsigned char *remote_cli_test_telnet_tx(void)
{
    return s_host_tn_tx;
}

size_t remote_cli_test_telnet_tx_len(void)
{
    return s_host_tn_tx_len;
}

void remote_cli_start(void)
{
}

void remote_cli_suspend_for_ota(void)
{
    s_suspended_for_ota = true;
    remote_cli_host_end_session(false);
    s_remote_cli_reclaimed = true;
}

bool remote_cli_wait_suspended_for_ota(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return s_remote_cli_reclaimed;
}

void remote_cli_resume_after_ota(void)
{
    s_suspended_for_ota = false;
    s_remote_cli_reclaimed = false;
}

void remote_cli_poll_override(void)
{
    if (!s_session_active) {
        return;
    }

    if (!console_uart_rx_pending()) {
        return;
    }

    console_uart_consume_pending();
    remote_cli_host_end_session(true);
}

void remote_cli_request_disconnect(void)
{
    if (!s_session_active) {
        return;
    }

    remote_cli_host_end_session(false);
}

#endif /* HOST_TEST */

#else /* !REMOTE_CLI_ENABLE */

void remote_cli_start(void)
{
}

void remote_cli_poll_override(void)
{
}

void remote_cli_request_disconnect(void)
{
}

void remote_cli_suspend_for_ota(void)
{
}

bool remote_cli_wait_suspended_for_ota(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return true;
}

void remote_cli_resume_after_ota(void)
{
}

#endif /* REMOTE_CLI_ENABLE */
