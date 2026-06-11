/*
 * UART CLI: adc command table — spec/30-processes/uart-console.md
 */

#include <stdio.h>
#include "app_log.h"
#include <stdlib.h>
#include <string.h>

#include "adc_cli.h"
#include "adc_limits.h"
#include "app_adc_cli.h"

static uint8_t adc_cli_read_motor(void)
{
    uint16_t ma;
    port_err_t err;

    err = adc_cli_run_motor_read(&ma);
    if (err != PORT_OK) {
        adc_cli_print_fail(err);
        return 1;
    }

    app_log_info("cli", "adc motor: %u mA", (unsigned)ma);
    return 0;
}

static uint8_t adc_cli_read_battery(void)
{
    uint16_t mv;
    port_err_t err;

    err = adc_cli_run_battery_read(&mv);
    if (err != PORT_OK) {
        adc_cli_print_fail(err);
        return 1;
    }

    app_log_info("cli", "adc battery: %u mV", (unsigned)mv);
    return 0;
}

static uint8_t adc_cli_read_cmd(uint8_t argc, char *argv[])
{
    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: adc read motor|battery");
        return 1;
    }

    if (strcmp(argv[0], "motor") == 0) {
        return adc_cli_read_motor();
    }

    if (strcmp(argv[0], "battery") == 0) {
        return adc_cli_read_battery();
    }

    app_log_info("cli", "usage: adc read motor|battery");
    return 1;
}

static uint8_t adc_cli_cal_capture_cmd(uint8_t argc, char *argv[])
{
    char ratio[32];
    adc_cal_status_t status;
    unsigned long true_mv;
    port_err_t err;

    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: adc cal <true_mv>|status|reset");
        return 1;
    }

    true_mv = strtoul(argv[0], NULL, 10);
    if (true_mv < (unsigned long)ADC_CAL_TRUE_MV_MIN ||
        true_mv > (unsigned long)ADC_CAL_TRUE_MV_MAX) {
        adc_cli_print_cal_fail(PORT_ERR_INVALID_ARG);
        return 1;
    }

    err = adc_cli_run_cal_capture((uint16_t)true_mv);
    if (err != PORT_OK) {
        adc_cli_print_cal_fail(err);
        return 1;
    }

    err = adc_cli_run_cal_status(&status);
    if (err != PORT_OK) {
        adc_cli_print_cal_fail(err);
        return 1;
    }

    adc_cli_format_cal_status(&status, ratio, sizeof(ratio));
    app_log_info("cli", "adc cal ok (%s)", ratio);
    return 0;
}

static uint8_t adc_cli_cal_status_cmd(uint8_t argc, char *argv[])
{
    char ratio[32];
    adc_cal_status_t status;

    (void)argc;
    (void)argv;

    if (adc_cli_run_cal_status(&status) != PORT_OK) {
        adc_cli_print_cal_fail(PORT_ERR_IO);
        return 1;
    }

    adc_cli_format_cal_status(&status, ratio, sizeof(ratio));
    app_log_info("cli", "adc cal: %s", ratio);
    return 0;
}

static uint8_t adc_cli_cal_reset_cmd(uint8_t argc, char *argv[])
{
    port_err_t err;

    (void)argc;
    (void)argv;

    err = adc_cli_run_cal_reset();
    if (err != PORT_OK) {
        adc_cli_print_cal_fail(err);
        return 1;
    }

    app_log_info("cli", "adc cal reset ok");
    return 0;
}

static uint8_t adc_cli_cal_cmd(uint8_t argc, char *argv[])
{
    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: adc cal <true_mv>|status|reset");
        return 1;
    }

    if (strcmp(argv[0], "status") == 0) {
        return adc_cli_cal_status_cmd(argc - 1, argv + 1);
    }

    if (strcmp(argv[0], "reset") == 0) {
        return adc_cli_cal_reset_cmd(argc - 1, argv + 1);
    }

    return adc_cli_cal_capture_cmd(argc, argv);
}

cmd_t adc_cli_subcmds[] = {
    { "read", "adc read motor|battery", adc_cli_read_cmd, NULL },
    { "cal",  "adc cal <true_mv>|status|reset", adc_cli_cal_cmd, NULL },
    { NULL, NULL, NULL, NULL },
};
