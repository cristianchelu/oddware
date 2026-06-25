/*
 * UART CLI: feed command table — spec/30-processes/uart-console.md § feed
 */

#include <string.h>

#include "app_feed_cli.h"
#include "app_log.h"
#include "cli.h"
#include "feed_config.h"
#include "port_err.h"
#include "mqtt_feed_mode.h"

uint8_t feed_cli_run_mode_show(void)
{
    dispense_mode_t mode = feed_config_mode_get();

    app_log_info("cli", "feed mode: %s", feed_config_mode_string(mode));
    return 0;
}

uint8_t feed_cli_run_mode_set(dispense_mode_t mode)
{
    if (feed_config_mode_get() == mode) {
        app_log_info("cli", "feed mode: unchanged");
        return 1;
    }

    if (mqtt_feed_mode_apply(mode) != PORT_OK) {
        app_log_info("cli", "feed mode: nvdm write failed");
        return 1;
    }

    app_log_info("cli", "feed mode ok");
    return 0;
}

static uint8_t feed_cli_mode(uint8_t argc, char *argv[])
{
    if (argc < 1u || argv[0] == NULL) {
        return feed_cli_run_mode_show();
    }

    if (strcmp(argv[0], "open_loop") == 0) {
        return feed_cli_run_mode_set(DISPENSE_MODE_OPEN_LOOP);
    }

    if (strcmp(argv[0], "compensated") == 0) {
        return feed_cli_run_mode_set(DISPENSE_MODE_COMPENSATED);
    }

    app_log_info("cli", "usage: feed mode [open_loop|compensated]");
    return 1;
}

cmd_t feed_cli_subcmds[] = {
    { "mode", "feed mode [open_loop|compensated]", feed_cli_mode, NULL },
    { NULL, NULL, NULL, NULL },
};
