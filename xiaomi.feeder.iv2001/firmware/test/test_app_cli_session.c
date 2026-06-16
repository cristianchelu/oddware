/* Tests: spec/30-processes/uart-console.md § Remote telnet console */

#include "unity.h"

#include "app_cli.h"
#include "app_cli_active.h"
#include "cli.h"

extern cli_t *cli_host_active(void);

static int uart_get(void)
{
    return -1;
}

static int uart_put(int c)
{
    return c;
}

static int remote_get(void)
{
    return 'x';
}

static int remote_put(int c)
{
    return c;
}

static cli_t s_uart_cli;
static cli_t s_remote_cli;
static cmd_t s_cmds[] = {
    { NULL, NULL, NULL, NULL },
};

/* Regression: remote_cli_run_session must return MiniCLI to UART0 after teardown. */
void test_app_cli_restore_local_rebinds_uart_after_remote(void)
{
    s_uart_cli.state = 1;
    s_uart_cli.echo  = 0;
    s_uart_cli.get   = uart_get;
    s_uart_cli.put   = uart_put;
    s_uart_cli.cmd   = s_cmds;

    s_remote_cli.state = 1;
    s_remote_cli.echo  = 0;
    s_remote_cli.get   = remote_get;
    s_remote_cli.put   = remote_put;
    s_remote_cli.cmd   = s_cmds;

    app_cli_set_uart_cli(&s_uart_cli);
    cli_init(&s_uart_cli);
    TEST_ASSERT_EQUAL_PTR(&s_uart_cli, cli_host_active());
    TEST_ASSERT_EQUAL_PTR(&s_uart_cli, app_cli_test_active_cli());

    app_cli_session_init(&s_remote_cli, remote_get, remote_put);
    TEST_ASSERT_EQUAL_PTR(&s_remote_cli, cli_host_active());

    app_cli_restore_local();
    TEST_ASSERT_EQUAL_PTR(&s_uart_cli, cli_host_active());
    TEST_ASSERT_EQUAL_PTR(&s_uart_cli, app_cli_test_active_cli());
}
