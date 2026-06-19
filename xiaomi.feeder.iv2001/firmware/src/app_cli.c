/*
 * UART console — spec/30-processes/uart-console.md
 */

#include <stdio.h>
#include "app_log.h"
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "app_cli.h"
#include "app_cli_active.h"
#include "app_cli_ota.h"
#include "io_def.h"
#include "task_def.h"

#include "hal_cache.h"
#include "hal_sys.h"

#include "boot_bank_target.h"
#include "app_display_cli.h"
#include "app_adc_cli.h"
#include "app_hopper_cli.h"
#include "app_power_cli.h"
#include "app_index_cli.h"
#include "app_weigh_cli.h"
#include "app_motor_cli.h"
#include "app_dispense_cli.h"
#include "dispense_cli.h"
#include "app_wifi_cli.h"
#include "app_mqtt_cli.h"
#include "app_time_cli.h"
#include "provision.h"

#if REMOTE_CLI_ENABLE
#include "console_mux.h"
#include "remote_cli.h"
#endif

#define CLI_HISTORY_LINES   20
#define CLI_HISTORY_LINE    128

static char s_history_lines[CLI_HISTORY_LINES][CLI_HISTORY_LINE];
static char *s_history_ptrs[CLI_HISTORY_LINES];
static char s_history_input[CLI_HISTORY_LINE];
static char s_history_parse_token[CLI_HISTORY_LINE];

static uint8_t app_cli_bank_show(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    boot_bank_t active = boot_bank_query_active();
    bool unverified = boot_bank_query_unverified();
    uint8_t attempts = boot_bank_query_boot_attempts();

    app_log_info("cli", "active bank: %c", (active == BOOT_BANK_B) ? 'B' : 'A');
    app_log_info("cli", "unverified: %u", unverified ? 1u : 0u);
    app_log_info("cli", "boot_attempts: %u", (unsigned)attempts);
    return 0;
}

static uint8_t app_cli_bank_switch(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (boot_bank_switch_active() != 0) {
        app_log_info("cli", "bank switch failed");
        return 1;
    }

    app_log_info("cli", "bank switched — rebooting");
    vTaskDelay(200 / portTICK_PERIOD_MS);
    hal_cache_disable();
    hal_cache_deinit();
    hal_sys_reboot(HAL_SYS_REBOOT_MAGIC, WHOLE_SYSTEM_REBOOT_COMMAND);
    return 0;
}

static uint8_t app_cli_dispense(uint8_t argc, char *argv[])
{
    return dispense_cli_handle_default(argc, argv);
}

static uint8_t app_cli_remote_exit(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

#if REMOTE_CLI_ENABLE
    if (console_mux_remote_active()) {
        app_log_info("cli", "remote session ended");
        remote_cli_request_disconnect();
        return 0;
    }
#endif

    app_log_info("cli", "not in remote session");
    return 0;
}

static uint8_t app_cli_config_factory_reset(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (!provision_factory_reset()) {
        app_log_info("cli", "factory reset failed");
        return 1;
    }

    app_log_info("cli", "factory reset — rebooting");
    return 0;
}

static cmd_t app_cli_config_subcmds[] = {
    { "factory-reset", "erase all config and reboot", app_cli_config_factory_reset, NULL },
    { NULL, NULL, NULL, NULL },
};

static cmd_t app_cli_bank_subcmds[] = {
    { "show",   "show active bank",        app_cli_bank_show,   NULL },
    { "switch", "toggle A/B and reboot",   app_cli_bank_switch, NULL },
    { NULL, NULL, NULL, NULL },
};

static cmd_t app_cli_cmds[] = {
    { "bank",   "bank show|switch",           NULL, app_cli_bank_subcmds },
    { "wifi",   "wifi show|set|connect",      NULL, wifi_cli_subcmds },
    { "mqtt",   "mqtt show|set|connect",      NULL, mqtt_cli_subcmds },
    { "time",   "time show|sync",             NULL, time_cli_subcmds },
    { "display", "display test|fill|off",     NULL, display_cli_subcmds },
    { "weigh",   "weigh power|read|cal",      NULL, weigh_cli_subcmds },
    { "index",   "index read",                NULL, index_cli_subcmds },
    { "hopper",  "hopper read",               NULL, hopper_cli_subcmds },
    { "power",   "power show",                NULL, power_cli_subcmds },
    { "adc",     "adc read|cal",              NULL, adc_cli_subcmds },
    { "motor",   "motor fwd|rev <ms>|park",   NULL, motor_cli_subcmds },
    { "dispense", "dispense [portions <N>]", app_cli_dispense, dispense_cli_subcmds },
    { "config", "config factory-reset",       NULL, app_cli_config_subcmds },
    { "exit",   "end remote session (no-op on UART)", app_cli_remote_exit, NULL },
    { NULL, NULL, NULL, NULL },
};

static cli_t s_cli = {
    .state = 1,
    .echo  = 0,
    .get   = __io_getchar,
    .put   = __io_putchar,
    .cmd   = app_cli_cmds,
};

static TaskHandle_t s_app_cli_task;
static volatile bool s_suspended_for_ota;
static volatile bool s_task_reclaimed;

static void app_cli_task(void *param);

static void app_cli_enter_ota_suspend(void)
{
    s_task_reclaimed = true;
    s_app_cli_task = NULL;
    vTaskDelete(NULL);
}

static bool app_cli_wait_suspended(uint32_t timeout_ms)
{
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

    while (xTaskGetTickCount() < deadline) {
        if (s_task_reclaimed) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return s_task_reclaimed;
}

static bool app_cli_spawn_task(void)
{
    if (s_app_cli_task != NULL) {
        return true;
    }

    s_task_reclaimed = false;

    if (xTaskCreate(app_cli_task,
                    "app_cli",
                    APP_TASK_STACKSIZE / sizeof(portSTACK_TYPE),
                    NULL,
                    APP_TASK_PRIO,
                    &s_app_cli_task) != pdPASS) {
        app_log_error("cli", "uart console: task create failed");
        return false;
    }

    return true;
}

static void app_cli_history_bind(cli_t *cli,
                                 char (*lines)[CLI_HISTORY_LINE],
                                 char **ptrs,
                                 char *input,
                                 char *parse_token)
{
    cli_history_t *hist = &cli->history;
    int i;

    for (i = 0; i < CLI_HISTORY_LINES; i++) {
        ptrs[i] = lines[i];
    }

    hist->history     = ptrs;
    hist->input       = input;
    hist->parse_token = parse_token;
    hist->history_max = CLI_HISTORY_LINES;
    hist->line_max    = CLI_HISTORY_LINE;
    hist->index       = 0;
    hist->position    = 0;
    hist->full        = 0;
}

void app_cli_session_init(cli_t *cli, int (*get)(void), int (*put)(int))
{
    static char remote_lines[CLI_HISTORY_LINES][CLI_HISTORY_LINE];
    static char *remote_ptrs[CLI_HISTORY_LINES];
    static char remote_input[CLI_HISTORY_LINE];
    static char remote_parse_token[CLI_HISTORY_LINE];

    app_cli_history_bind(cli, remote_lines, remote_ptrs,
                         remote_input, remote_parse_token);

    cli->state = 1;
    cli->echo  = 0;
    cli->get   = get;
    cli->put   = put;
    cli->cmd   = app_cli_cmds;
    cli_init(cli);
}

static void app_cli_task(void *param)
{
    (void)param;

    app_cli_history_bind(&s_cli, s_history_lines, s_history_ptrs,
                         s_history_input, s_history_parse_token);
    s_cli.state = 1;
    s_cli.echo  = 0;
    s_cli.get   = __io_getchar;
    s_cli.put   = __io_putchar;
    s_cli.cmd   = app_cli_cmds;
    app_cli_set_uart_cli(&s_cli);
    cli_init(&s_cli);

#if REMOTE_CLI_ENABLE
    console_mux_init();
#endif

    for (;;) {
        if (s_suspended_for_ota) {
            app_cli_enter_ota_suspend();
        }

#if REMOTE_CLI_ENABLE
        if (console_mux_remote_active()) {
            remote_cli_poll_override();
            vTaskDelay(1);
        } else {
            app_cli_restore_local();
            cli_task();
        }
#else
        cli_task();
#endif
    }
}

void app_cli_start(void)
{
    (void)app_cli_spawn_task();
}

void app_cli_suspend_for_ota(void)
{
    TaskHandle_t task;

    s_suspended_for_ota = true;

    if (app_cli_wait_suspended(2000u)) {
        return;
    }

    task = s_app_cli_task;
    if (task != NULL) {
        app_log_warn("cli", "uart console: force delete for ota");
        s_app_cli_task = NULL;
        s_task_reclaimed = true;
        vTaskDelete(task);
    }
}

void app_cli_resume_after_ota(void)
{
    s_suspended_for_ota = false;
    s_task_reclaimed = false;

    if (s_app_cli_task == NULL) {
        (void)app_cli_spawn_task();
    }
}
