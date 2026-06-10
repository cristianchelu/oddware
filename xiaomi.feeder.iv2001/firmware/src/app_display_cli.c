/*
 * UART CLI: display command table — spec/30-processes/uart-console.md
 */

#include <stdio.h>

#include "cli.h"
#include "display_cli.h"
#include "app_display_cli.h"

static uint8_t display_cli_test_cmd(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (display_cli_run_test() != PORT_OK) {
        printf("display test failed\r\n");
        return 1;
    }

    printf("display test ok\r\n");
    return 0;
}

static uint8_t display_cli_fill_cmd(uint8_t argc, char *argv[])
{
    uint8_t segment_byte;

    if (argc < 1 || argv[0] == NULL) {
        printf("usage: display fill <hex_byte>\r\n");
        return 1;
    }

    if (display_cli_parse_hex_byte(argv[0], &segment_byte) != PORT_OK) {
        printf("invalid hex byte\r\n");
        return 1;
    }

    if (display_cli_run_fill(segment_byte) != PORT_OK) {
        printf("display fill failed\r\n");
        return 1;
    }

    printf("display fill ok\r\n");
    return 0;
}

static uint8_t display_cli_off_cmd(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (display_cli_run_off() != PORT_OK) {
        printf("display off failed\r\n");
        return 1;
    }

    printf("display off ok\r\n");
    return 0;
}

cmd_t display_cli_subcmds[] = {
    { "test", "boot-style fill test via arbiter", display_cli_test_cmd, NULL },
    { "fill", "power on and show hex segment byte", display_cli_fill_cmd, NULL },
    { "off",  "power off display rail", display_cli_off_cmd, NULL },
    { NULL, NULL, NULL, NULL },
};
