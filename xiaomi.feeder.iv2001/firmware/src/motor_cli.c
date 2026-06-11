/*
 * Motor bench CLI logic — spec/30-processes/uart-console.md
 */

#include <stdio.h>
#include <stdint.h>

#include "motor_cli.h"
#include "motor_port.h"

void motor_cli_print_fail(const char *verb, port_err_t err)
{
    printf("motor %s failed (%s)\r\n", verb, port_err_name(err));
}

port_err_t motor_cli_parse_duration_ms(const char *text, uint32_t *duration_ms)
{
    uint32_t value = 0u;
    size_t i;

    if (text == NULL || duration_ms == NULL || text[0] == '\0') {
        return PORT_ERR_INVALID_ARG;
    }

    if (text[0] == '0') {
        return PORT_ERR_INVALID_ARG;
    }

    for (i = 0; text[i] != '\0'; i++) {
        if (text[i] < '0' || text[i] > '9') {
            return PORT_ERR_INVALID_ARG;
        }

        if (value > (UINT32_MAX - (uint32_t)(text[i] - '0')) / 10u) {
            return PORT_ERR_INVALID_ARG;
        }

        value = (value * 10u) + (uint32_t)(text[i] - '0');
    }

    if (value == 0u || value > MOTOR_CLI_MAX_MS) {
        return PORT_ERR_INVALID_ARG;
    }

    *duration_ms = value;
    return PORT_OK;
}

port_err_t motor_cli_run_fwd_ms(uint32_t duration_ms)
{
    const motor_port_t *port = motor_port_get();

    if (port == NULL || port->run_forward_ms == NULL) {
        return PORT_ERR_IO;
    }

    return port->run_forward_ms(duration_ms);
}

port_err_t motor_cli_run_rev_ms(uint32_t duration_ms)
{
    const motor_port_t *port = motor_port_get();

    if (port == NULL || port->run_reverse_ms == NULL) {
        return PORT_ERR_IO;
    }

    return port->run_reverse_ms(duration_ms);
}

uint8_t motor_cli_handle_run(const char *verb,
                               port_err_t (*run_ms)(uint32_t),
                               uint8_t argc, char *argv[])
{
    uint32_t duration_ms;
    port_err_t err;

    if (argc < 1 || argv == NULL || argv[0] == NULL) {
        printf("usage: motor %s <ms>\r\n", verb);
        return 1;
    }

    err = motor_cli_parse_duration_ms(argv[0], &duration_ms);
    if (err != PORT_OK) {
        printf("invalid duration\r\n");
        return 1;
    }

    err = run_ms(duration_ms);
    if (err != PORT_OK) {
        motor_cli_print_fail(verb, err);
        return 1;
    }

    printf("motor %s ok\r\n", verb);
    return 0;
}
