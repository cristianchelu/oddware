/*
 * Step 1 board bring-up — UART0 console checkpoint.
 * spec/40-architecture/build-integration.md
 */

#include "FreeRTOS.h"
#include "task.h"
#include "sys_init.h"
#include "syslog.h"
#include "system_mt7682.h"

log_create_module(main, PRINT_LEVEL_INFO);

int main(void)
{
    system_init();

    LOG_I(main, "FreeRTOS Running");

    SysInitStatus_Set();
    vTaskStartScheduler();

    for (;;) {
    }
}
