/*
 * UART CLI: motor command table — spec/30-processes/uart-console.md
 */

#include <stddef.h>

#include "cli.h"
#include "motor_cli.h"
#include "app_motor_cli.h"

static uint8_t motor_cli_fwd_cmd(uint8_t argc, char *argv[])
{
    return motor_cli_handle_run("fwd", motor_cli_run_fwd_ms, argc, argv);
}

static uint8_t motor_cli_rev_cmd(uint8_t argc, char *argv[])
{
    return motor_cli_handle_run("rev", motor_cli_run_rev_ms, argc, argv);
}

static uint8_t motor_cli_park_cmd(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return motor_cli_handle_park();
}

cmd_t motor_cli_subcmds[] = {
    { "fwd", "motor fwd <ms>", motor_cli_fwd_cmd, NULL },
    { "rev", "motor rev <ms>", motor_cli_rev_cmd, NULL },
    { "park", "motor park (async align)", motor_cli_park_cmd, NULL },
    { NULL, NULL, NULL, NULL },
};
