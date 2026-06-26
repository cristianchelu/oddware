/*
 * UART CLI: tare command — spec/30-processes/uart-console.md § tare
 */

#include "app_tare_cli.h"
#include "app_log.h"
#include "auto_tare.h"
#include "cli.h"
#include "weight_units.h"

uint8_t tare_cli_run_show(void)
{
    char formatted[16];

    if (auto_tare_stable_valid()) {
        (void)weight_format_cli_g(auto_tare_stable_dg(), formatted, sizeof(formatted));
        app_log_info("cli", "tare stable: %s g", formatted);
    } else {
        app_log_info("cli", "tare stable: (unset)");
    }

    (void)weight_format_cli_g(auto_tare_drift_offset_dg(), formatted, sizeof(formatted));
    app_log_info("cli", "tare drift: %s g", formatted);
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
