/*
 * Motor bench CLI logic — spec/30-processes/uart-console.md
 */

#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "app_log.h"
#include "motor_cli.h"
#include "motor_jam.h"
#include "motor_port.h"

void motor_cli_print_fail(const char *verb, port_err_t err)
{
    app_log_info("cli", "motor %s failed (%s)", verb, port_err_name(err));
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
        app_log_info("cli", "usage: motor %s <ms>", verb);
        return 1;
    }

    err = motor_cli_parse_duration_ms(argv[0], &duration_ms);
    if (err != PORT_OK) {
        app_log_info("cli", "invalid duration");
        return 1;
    }

    err = run_ms(duration_ms);
    if (err != PORT_OK) {
        motor_cli_print_fail(verb, err);
        return 1;
    }

    app_log_info("cli", "motor %s ok", verb);
    return 0;
}

typedef enum {
    MOTOR_PARK_IDLE = 0,
    MOTOR_PARK_WAIT,
} motor_park_cli_state_t;

static motor_park_cli_state_t s_park_state = MOTOR_PARK_IDLE;
static char s_park_test_line[48];

static void motor_cli_park_emit(const char *line)
{
    app_log_info("cli", "%s", line);
    (void)snprintf(s_park_test_line, sizeof(s_park_test_line), "%s", line);
}

uint8_t motor_cli_handle_park(void)
{
    const motor_port_t *port = motor_port_get();
    port_err_t err;

    if (port == NULL || port->request_park == NULL) {
        motor_cli_park_emit("motor park busy");
        return 1u;
    }

    if (port->is_active != NULL && port->is_active()) {
        motor_cli_park_emit("motor park busy");
        return 1u;
    }

    if (s_park_state != MOTOR_PARK_IDLE) {
        motor_cli_park_emit("motor park busy");
        return 1u;
    }

    err = port->request_park(MOTOR_PARK_MAX_PULSES_DEFAULT);
    if (err != PORT_OK) {
        motor_cli_park_emit("motor park busy");
        return 1u;
    }

    s_park_state = MOTOR_PARK_WAIT;
    motor_cli_park_emit("motor park started");
    return 0u;
}

void motor_cli_on_park_done(void)
{
    if (s_park_state != MOTOR_PARK_WAIT) {
        return;
    }

    s_park_state = MOTOR_PARK_IDLE;
    motor_cli_park_emit("motor park done");
}

void motor_cli_on_park_fault(void)
{
    if (s_park_state != MOTOR_PARK_WAIT) {
        return;
    }

    s_park_state = MOTOR_PARK_IDLE;
    motor_cli_park_emit("motor park fault: stuck");
}

void motor_cli_test_reset_park(void)
{
    s_park_state = MOTOR_PARK_IDLE;
    s_park_test_line[0] = '\0';
}

bool motor_cli_test_take_park_line(char *buf, size_t len)
{
    size_t n;

    if (buf == NULL || len == 0u || s_park_test_line[0] == '\0') {
        return false;
    }

    n = strlen(s_park_test_line);
    if (n >= len) {
        n = len - 1u;
    }

    memcpy(buf, s_park_test_line, n);
    buf[n] = '\0';
    s_park_test_line[0] = '\0';
    return true;
}
