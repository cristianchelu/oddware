/*
 * Step 2 — dual-image boot + UART CLI bank switch checkpoint.
 * spec/40-architecture/build-integration.md
 */

#include "FreeRTOS.h"
#include "task.h"
#include "sys_init.h"
#include "syslog.h"
#include "system_mt7682.h"

#include "boot_bank_cli.h"
#include "boot_bank_target.h"

log_create_module(main, PRINT_LEVEL_INFO);

int main(void)
{
    system_init();

    LOG_I(main, "FreeRTOS Running (bank %c)",
          (boot_bank_query_active() == BOOT_BANK_B) ? 'B' : 'A');

    boot_bank_cli_start();

    SysInitStatus_Set();
    vTaskStartScheduler();

    for (;;) {
    }
}
