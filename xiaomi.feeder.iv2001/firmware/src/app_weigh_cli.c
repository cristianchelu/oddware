/*
 * UART CLI: weigh command table — spec/30-processes/uart-console.md
 */

#include <stdio.h>
#include "app_log.h"
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "weigh_cli.h"
#include "app_weigh_cli.h"
#include "weight_units.h"

static uint8_t weigh_cli_power_cmd(uint8_t argc, char *argv[])
{
    port_err_t err;

    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: weigh power on|off");
        return 1;
    }

    if (strcmp(argv[0], "on") == 0) {
        err = weigh_cli_run_power_on();
        if (err != PORT_OK) {
            weigh_cli_print_fail("power on", err);
            return 1;
        }
        app_log_info("cli", "weigh power on ok");
        return 0;
    }

    if (strcmp(argv[0], "off") == 0) {
        err = weigh_cli_run_power_off();
        if (err != PORT_OK) {
            weigh_cli_print_fail("power off", err);
            return 1;
        }
        app_log_info("cli", "weigh power off ok");
        return 0;
    }

    app_log_info("cli", "usage: weigh power on|off");
    return 1;
}

static uint8_t weigh_cli_read_cmd(uint8_t argc, char *argv[])
{
    weight_dg_t dg;
    char formatted[16];
    port_err_t err;

    (void)argc;
    (void)argv;

    err = weigh_cli_run_read(&dg);
    if (err == PORT_OK) {
        (void)weight_format_cli_g(dg, formatted, sizeof(formatted));
        app_log_info("cli", "weight: %s g", formatted);
        return 0;
    }

    if (weigh_cli_print_read_fail(err)) {
        int32_t raw;

        err = weigh_cli_run_read_raw(&raw);
        if (err == PORT_OK) {
            app_log_info("cli", "weight: %ld g (raw, no calibration)", (long)raw);
            return 0;
        }
        return 1;
    }

    weigh_cli_print_fail("read", err);
    return 1;
}

static uint8_t weigh_cli_cal_zero_cmd(uint8_t argc, char *argv[])
{
    port_err_t err;

    (void)argc;
    (void)argv;

    err = weigh_cli_run_cal_zero();
    if (err != PORT_OK) {
        weigh_cli_print_fail("cal zero", err);
        return 1;
    }

    app_log_info("cli", "weigh cal zero ok");
    app_log_info("cli", "install provided bowl, then: weigh cal span");
    return 0;
}

static uint8_t weigh_cli_cal_span_cmd(uint8_t argc, char *argv[])
{
    port_err_t err;

    (void)argc;
    (void)argv;

    err = weigh_cli_run_cal_span();
    if (err != PORT_OK) {
        weigh_cli_print_fail("cal span", err);
        return 1;
    }

    app_log_info("cli", "weigh cal span ok");
    return 0;
}

static uint8_t weigh_cli_cal_status_cmd(uint8_t argc, char *argv[])
{
    weight_cal_status_t st;

    (void)argc;
    (void)argv;

    st = weigh_cli_run_cal_status();
    app_log_info("cli", "weigh cal: %s", weigh_cli_cal_status_name(st));
    return 0;
}

static uint8_t weigh_cli_cal_cmd(uint8_t argc, char *argv[])
{
    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: weigh cal zero|span|status");
        return 1;
    }

    if (strcmp(argv[0], "zero") == 0) {
        return weigh_cli_cal_zero_cmd(argc - 1, argv + 1);
    }
    if (strcmp(argv[0], "span") == 0) {
        return weigh_cli_cal_span_cmd(argc - 1, argv + 1);
    }
    if (strcmp(argv[0], "status") == 0) {
        return weigh_cli_cal_status_cmd(argc - 1, argv + 1);
    }

    app_log_info("cli", "usage: weigh cal zero|span|status");
    return 1;
}

cmd_t weigh_cli_subcmds[] = {
    { "power",   "weigh power on|off",         weigh_cli_power_cmd,   NULL },
    { "read",    "read bowl weight",          weigh_cli_read_cmd,    NULL },
    { "cal",     "weigh cal zero|span|status", weigh_cli_cal_cmd,    NULL },
    { NULL, NULL, NULL, NULL },
};
