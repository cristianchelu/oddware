/*
 * UART CLI: tare command — spec/30-processes/uart-console.md § tare
 */

#include "app_tare_cli.h"
#include "app_log.h"
#include "auto_tare.h"
#include "cli.h"

uint8_t tare_cli_run_show(void)
{
    if (auto_tare_stable_valid()) {
        app_log_info("cli", "tare stable: %ld g", (long)auto_tare_stable_grams());
    } else {
        app_log_info("cli", "tare stable: (unset)");
    }

    app_log_info("cli", "tare drift: %ld g", (long)auto_tare_drift_offset_g());
    app_log_info("cli",
                 "tare pending: %s",
                 auto_tare_pending_calibration() ? "yes" : "no");
    return 0;
}

static uint8_t tare_cli_show(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    return tare_cli_run_show();
}

cmd_t tare_cli_subcmds[] = {
    { "show", "tare show", tare_cli_show, NULL },
    { NULL, NULL, NULL, NULL },
};
