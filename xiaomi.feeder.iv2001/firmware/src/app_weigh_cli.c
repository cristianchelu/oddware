/*
 * UART CLI: weigh command table — spec/30-processes/uart-console.md
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "cli.h"
#include "weigh_cli.h"
#include "app_weigh_cli.h"

static uint8_t weigh_cli_power_cmd(uint8_t argc, char *argv[])
{
    port_err_t err;

    if (argc < 1 || argv[0] == NULL) {
        printf("usage: weigh power on|off\r\n");
        return 1;
    }

    if (strcmp(argv[0], "on") == 0) {
        err = weigh_cli_run_power_on();
        if (err != PORT_OK) {
            weigh_cli_print_fail("power on", err);
            return 1;
        }
        printf("weigh power on ok\r\n");
        return 0;
    }

    if (strcmp(argv[0], "off") == 0) {
        err = weigh_cli_run_power_off();
        if (err != PORT_OK) {
            weigh_cli_print_fail("power off", err);
            return 1;
        }
        printf("weigh power off ok\r\n");
        return 0;
    }

    printf("usage: weigh power on|off\r\n");
    return 1;
}

static uint8_t weigh_cli_read_cmd(uint8_t argc, char *argv[])
{
    int32_t grams;
    port_err_t err;

    (void)argc;
    (void)argv;

    err = weigh_cli_run_read(&grams);
    if (err == PORT_OK) {
        printf("weight: %ld g\r\n", (long)grams);
        return 0;
    }

    if (weigh_cli_print_read_fail(err)) {
        err = weigh_cli_run_read_raw(&grams);
        if (err == PORT_OK) {
            printf("weight: %ld g (raw, no calibration)\r\n", (long)grams);
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

    printf("weigh cal zero ok\r\n");
    printf("install provided bowl, then: weigh cal span\r\n");
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

    printf("weigh cal span ok\r\n");
    return 0;
}

static uint8_t weigh_cli_cal_status_cmd(uint8_t argc, char *argv[])
{
    weight_cal_status_t st;

    (void)argc;
    (void)argv;

    st = weigh_cli_run_cal_status();
    printf("weigh cal: %s\r\n", weigh_cli_cal_status_name(st));
    return 0;
}

static uint8_t weigh_cli_cal_cmd(uint8_t argc, char *argv[])
{
    if (argc < 1 || argv[0] == NULL) {
        printf("usage: weigh cal zero|span|status\r\n");
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

    printf("usage: weigh cal zero|span|status\r\n");
    return 1;
}

cmd_t weigh_cli_subcmds[] = {
    { "power",   "weigh power on|off",         weigh_cli_power_cmd,   NULL },
    { "read",    "read bowl weight",          weigh_cli_read_cmd,    NULL },
    { "cal",     "weigh cal zero|span|status", weigh_cli_cal_cmd,    NULL },
    { NULL, NULL, NULL, NULL },
};
