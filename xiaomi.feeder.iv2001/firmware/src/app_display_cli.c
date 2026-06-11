/*
 * UART CLI: display command table — spec/30-processes/uart-console.md
 */

#include <stdio.h>
#include "app_log.h"
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "display_cli.h"
#include "app_display_cli.h"

static uint8_t display_cli_test_cmd(uint8_t argc, char *argv[])
{
    port_err_t err;

    (void)argc;
    (void)argv;

    err = display_cli_run_test();
    if (err != PORT_OK) {
        display_cli_print_fail("test", err);
        return 1;
    }

    app_log_info("cli", "display test ok");
    return 0;
}

static uint8_t display_cli_fill_cmd(uint8_t argc, char *argv[])
{
    uint8_t segment_byte;
    port_err_t err;

    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: display fill <hex_byte>");
        return 1;
    }

    if (display_cli_parse_hex_byte(argv[0], &segment_byte) != PORT_OK) {
        app_log_info("cli", "invalid hex byte");
        return 1;
    }

    err = display_cli_run_fill(segment_byte);
    if (err != PORT_OK) {
        display_cli_print_fail("fill", err);
        return 1;
    }

    app_log_info("cli", "display fill ok");
    return 0;
}

static uint8_t display_cli_off_cmd(uint8_t argc, char *argv[])
{
    port_err_t err;

    (void)argc;
    (void)argv;

    err = display_cli_run_off();
    if (err != PORT_OK) {
        display_cli_print_fail("off", err);
        return 1;
    }

    app_log_info("cli", "display off ok");
    return 0;
}

static uint8_t display_cli_number_cmd(uint8_t argc, char *argv[])
{
    uint16_t value;
    display_unit_t unit = DISPLAY_UNIT_NONE;
    port_err_t err;

    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: display number <0-999> [g|%%]");
        return 1;
    }

    if (display_cli_parse_number(argv[0], &value) != PORT_OK) {
        app_log_info("cli", "invalid number");
        return 1;
    }

    if (argc >= 2 && argv[1] != NULL) {
        if (strcmp(argv[1], "g") == 0) {
            unit = DISPLAY_UNIT_GRAM;
        } else if (strcmp(argv[1], "%") == 0) {
            unit = DISPLAY_UNIT_PERCENT;
        }
    }

    err = display_cli_run_number(value, unit);
    if (err != PORT_OK) {
        display_cli_print_fail("number", err);
        return 1;
    }

    app_log_info("cli", "display number ok");
    return 0;
}

static uint8_t display_cli_icon_cmd(uint8_t argc, char *argv[])
{
    display_icon_t icon;
    uint16_t on_ms;
    uint16_t off_ms;
    port_err_t err;

    if (argc < 2 || argv[0] == NULL || argv[1] == NULL) {
        app_log_info("cli", "usage: display icon <name> on|off|blink|steady ...");
        return 1;
    }

    if (!display_presentation_parse_icon(argv[0], &icon)) {
        app_log_info("cli", "unknown icon");
        return 1;
    }

    if (strcmp(argv[1], "on") == 0) {
        err = display_cli_run_icon_set(icon, true);
        if (err != PORT_OK) {
            display_cli_print_fail("icon", err);
            return 1;
        }
        app_log_info("cli", "display icon ok");
        return 0;
    }

    if (strcmp(argv[1], "off") == 0) {
        err = display_cli_run_icon_set(icon, false);
        if (err != PORT_OK) {
            display_cli_print_fail("icon", err);
            return 1;
        }
        app_log_info("cli", "display icon ok");
        return 0;
    }

    if (strcmp(argv[1], "blink") == 0) {
        if (argc < 4 || argv[2] == NULL || argv[3] == NULL) {
            app_log_info("cli", "usage: display icon <name> blink <on_ms> <off_ms>");
            return 1;
        }
        if (display_cli_parse_blink_ms(argv[2], &on_ms) != PORT_OK ||
            display_cli_parse_blink_ms(argv[3], &off_ms) != PORT_OK ||
            on_ms < DISPLAY_PRESENTATION_BLINK_MIN_MS ||
            off_ms < DISPLAY_PRESENTATION_BLINK_MIN_MS) {
            app_log_info("cli", "invalid blink timing");
            return 1;
        }
        err = display_cli_run_icon_blink(icon, on_ms, off_ms);
        if (err != PORT_OK) {
            display_cli_print_fail("icon blink", err);
            return 1;
        }
        app_log_info("cli", "display icon blink ok");
        return 0;
    }

    if (strcmp(argv[1], "steady") == 0) {
        err = display_cli_run_icon_steady(icon);
        if (err != PORT_OK) {
            display_cli_print_fail("icon steady", err);
            return 1;
        }
        app_log_info("cli", "display icon steady ok");
        return 0;
    }

    app_log_info("cli", "usage: display icon <name> on|off|blink|steady ...");
    return 1;
}

static uint8_t display_cli_anim_cmd(uint8_t argc, char *argv[])
{
    display_builtin_anim_t id;
    bool loop = false;
    port_err_t err;

    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: display anim <ota|lock> [loop]");
        return 1;
    }

    if (strcmp(argv[0], "stop") == 0) {
        err = display_cli_run_anim_stop();
        if (err != PORT_OK) {
            display_cli_print_fail("anim stop", err);
            return 1;
        }
        app_log_info("cli", "display anim stop ok");
        return 0;
    }

    if (!display_presentation_parse_builtin_anim(argv[0], &id)) {
        app_log_info("cli", "unknown animation");
        return 1;
    }

    if (argc >= 2 && argv[1] != NULL && strcmp(argv[1], "loop") == 0) {
        loop = true;
    }

    err = display_cli_run_anim(id, loop);
    if (err != PORT_OK) {
        display_cli_print_fail("anim", err);
        return 1;
    }

    app_log_info("cli", "display anim ok");
    return 0;
}

static uint8_t display_cli_brightness_cmd(uint8_t argc, char *argv[])
{
    char *end = NULL;
    unsigned long level;
    port_err_t err;

    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: display brightness <1-4>");
        return 1;
    }

    level = strtoul(argv[0], &end, 10);
    if (end == argv[0] || *end != '\0' || level < 1u || level > 4u) {
        app_log_info("cli", "invalid brightness");
        return 1;
    }

    err = display_cli_run_brightness((uint8_t)level);
    if (err != PORT_OK) {
        display_cli_print_fail("brightness", err);
        return 1;
    }

    app_log_info("cli", "display brightness ok");
    return 0;
}

cmd_t display_cli_subcmds[] = {
    { "test",       "boot-style fill test via arbiter", display_cli_test_cmd, NULL },
    { "fill",       "power on and show hex segment byte", display_cli_fill_cmd, NULL },
    { "off",        "power off display rail", display_cli_off_cmd, NULL },
    { "number",     "show 0-999 with optional unit", display_cli_number_cmd, NULL },
    { "icon",       "set or blink pictograph by name", display_cli_icon_cmd, NULL },
    { "anim",       "play built-in animation", display_cli_anim_cmd, NULL },
    { "brightness", "set brightness level 1-4", display_cli_brightness_cmd, NULL },
    { NULL, NULL, NULL, NULL },
};
