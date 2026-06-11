/*
 * UART console — spec/30-processes/uart-console.md
 */

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "cli.h"
#include "io_def.h"
#include "task_def.h"

#include "hal_cache.h"
#include "hal_sys.h"

#include "boot_bank_target.h"
#include "app_display_cli.h"
#include "app_adc_cli.h"
#include "app_hopper_cli.h"
#include "app_index_cli.h"
#include "app_weigh_cli.h"
#include "app_motor_cli.h"
#include "app_wifi_cli.h"
#include "app_mqtt_cli.h"
#include "provision.h"

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

    printf("active bank: %c\r\n", (active == BOOT_BANK_B) ? 'B' : 'A');
    return 0;
}

static uint8_t app_cli_bank_switch(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (boot_bank_switch_active() != 0) {
        printf("bank switch failed\r\n");
        return 1;
    }

    printf("bank switched — rebooting\r\n");
    vTaskDelay(200 / portTICK_PERIOD_MS);
    hal_cache_disable();
    hal_cache_deinit();
    hal_sys_reboot(HAL_SYS_REBOOT_MAGIC, WHOLE_SYSTEM_REBOOT_COMMAND);
    return 0;
}

static uint8_t app_cli_config_factory_reset(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (!provision_factory_reset()) {
        printf("factory reset failed\r\n");
        return 1;
    }

    printf("factory reset — rebooting\r\n");
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
    { "display", "display test|fill|off",     NULL, display_cli_subcmds },
    { "weigh",   "weigh power|read|cal",      NULL, weigh_cli_subcmds },
    { "index",   "index read",                NULL, index_cli_subcmds },
    { "hopper",  "hopper read",               NULL, hopper_cli_subcmds },
    { "adc",     "adc read|cal",              NULL, adc_cli_subcmds },
    { "motor",   "motor fwd|rev <ms>",        NULL, motor_cli_subcmds },
    { "config", "config factory-reset",       NULL, app_cli_config_subcmds },
    { NULL, NULL, NULL, NULL },
};

static cli_t s_cli = {
    .state = 1,
    .echo  = 0,
    .get   = __io_getchar,
    .put   = __io_putchar,
    .cmd   = app_cli_cmds,
};

static void app_cli_task(void *param)
{
    cli_history_t *hist = &s_cli.history;
    int i;

    (void)param;

    for (i = 0; i < CLI_HISTORY_LINES; i++) {
        s_history_ptrs[i] = s_history_lines[i];
    }

    hist->history     = s_history_ptrs;
    hist->input       = s_history_input;
    hist->parse_token = s_history_parse_token;
    hist->history_max = CLI_HISTORY_LINES;
    hist->line_max    = CLI_HISTORY_LINE;
    hist->index       = 0;
    hist->position    = 0;
    hist->full        = 0;

    cli_init(&s_cli);

    for (;;) {
        cli_task();
    }
}

void app_cli_start(void)
{
    xTaskCreate(app_cli_task,
                "app_cli",
                APP_TASK_STACKSIZE / sizeof(portSTACK_TYPE),
                NULL,
                APP_TASK_PRIO,
                NULL);
}
