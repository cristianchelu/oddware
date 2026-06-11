/*
 * UART CLI: adc command table — spec/30-processes/uart-console.md
 */

#include <stdio.h>
#include <string.h>

#include "adc_cli.h"
#include "app_adc_cli.h"

static uint8_t adc_cli_read_motor(void)
{
    uint16_t mv;
    port_err_t err;

    err = adc_cli_run_motor_read(&mv);
    if (err != PORT_OK) {
        adc_cli_print_fail(err);
        return 1;
    }

    printf("adc motor: %u mV\r\n", (unsigned)mv);
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

    printf("adc battery: %u mV\r\n", (unsigned)mv);
    return 0;
}

static uint8_t adc_cli_read_cmd(uint8_t argc, char *argv[])
{
    if (argc < 1 || argv[0] == NULL) {
        printf("usage: adc read motor|battery\r\n");
        return 1;
    }

    if (strcmp(argv[0], "motor") == 0) {
        return adc_cli_read_motor();
    }

    if (strcmp(argv[0], "battery") == 0) {
        return adc_cli_read_battery();
    }

    printf("usage: adc read motor|battery\r\n");
    return 1;
}

cmd_t adc_cli_subcmds[] = {
    { "read", "adc read motor|battery", adc_cli_read_cmd, NULL },
    { NULL, NULL, NULL, NULL },
};
