/*
 * UART CLI: show / switch A/B boot bank (Step 2 checkpoint).
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

#define CLI_HISTORY_LINES   20
#define CLI_HISTORY_LINE    128

static char s_history_lines[CLI_HISTORY_LINES][CLI_HISTORY_LINE];
static char *s_history_ptrs[CLI_HISTORY_LINES];
static char s_history_input[CLI_HISTORY_LINE];
static char s_history_parse_token[CLI_HISTORY_LINE];

static uint8_t boot_bank_cli_show(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    boot_bank_t active = boot_bank_query_active();

    printf("active bank: %c\r\n", (active == BOOT_BANK_B) ? 'B' : 'A');
    return 0;
}

static uint8_t boot_bank_cli_switch(uint8_t argc, char *argv[])
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

static cmd_t boot_bank_subcmds[] = {
    { "show",   "show active bank",   boot_bank_cli_show,   NULL },
    { "switch", "toggle A/B and reboot", boot_bank_cli_switch, NULL },
    { NULL, NULL, NULL, NULL },
};

static cmd_t boot_bank_cmds[] = {
    { "bank", "bank show|switch", NULL, boot_bank_subcmds },
    { NULL, NULL, NULL, NULL },
};

static cli_t s_cli = {
    .state = 1,
    .echo  = 0,
    .get   = __io_getchar,
    .put   = __io_putchar,
    .cmd   = boot_bank_cmds,
};

static void boot_bank_cli_task(void *param)
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

void boot_bank_cli_start(void)
{
    xTaskCreate(boot_bank_cli_task,
                "bank_cli",
                APP_TASK_STACKSIZE / sizeof(portSTACK_TYPE),
                NULL,
                APP_TASK_PRIO,
                NULL);
}
