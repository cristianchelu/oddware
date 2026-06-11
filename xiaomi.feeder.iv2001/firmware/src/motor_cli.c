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

typedef enum {
    MOTOR_TIMED_IDLE = 0,
    MOTOR_TIMED_WAIT,
} motor_timed_cli_state_t;

static motor_timed_cli_state_t s_timed_state = MOTOR_TIMED_IDLE;
static char s_timed_verb[8];
static char s_timed_test_line[48];

static void motor_cli_timed_emit(const char *line)
{
    app_log_info("cli", "%s", line);
    (void)snprintf(s_timed_test_line, sizeof(s_timed_test_line), "%s", line);
}

static uint8_t motor_cli_handle_timed_run(const char *verb,
                                          port_err_t (*request_ms)(uint32_t),
                                          uint8_t argc, char *argv[])
{
    uint32_t duration_ms;
    port_err_t err;
    const motor_port_t *port = motor_port_get();

    if (argc < 1 || argv == NULL || argv[0] == NULL) {
        app_log_info("cli", "usage: motor %s <ms>", verb);
        return 1;
    }

    err = motor_cli_parse_duration_ms(argv[0], &duration_ms);
    if (err != PORT_OK) {
        app_log_info("cli", "invalid duration");
        return 1;
    }

    if (port == NULL || request_ms == NULL) {
        motor_cli_print_fail(verb, PORT_ERR_IO);
        return 1;
    }

    if (port->is_active != NULL && port->is_active()) {
        motor_cli_print_fail(verb, PORT_ERR_BUSY);
        return 1;
    }

    if (s_timed_state != MOTOR_TIMED_IDLE) {
        motor_cli_print_fail(verb, PORT_ERR_BUSY);
        return 1;
    }

    err = request_ms(duration_ms);
    if (err != PORT_OK) {
        motor_cli_print_fail(verb, err);
        return 1;
    }

    (void)snprintf(s_timed_verb, sizeof(s_timed_verb), "%s", verb);
    s_timed_state = MOTOR_TIMED_WAIT;
    motor_cli_timed_emit(
        (strcmp(verb, "fwd") == 0) ? "motor fwd started" : "motor rev started");
    return 0;
}

uint8_t motor_cli_handle_fwd(uint8_t argc, char *argv[])
{
    const motor_port_t *port = motor_port_get();

    if (port == NULL || port->request_timed_forward_ms == NULL) {
        motor_cli_print_fail("fwd", PORT_ERR_IO);
        return 1u;
    }

    return motor_cli_handle_timed_run("fwd", port->request_timed_forward_ms,
                                      argc, argv);
}

uint8_t motor_cli_handle_rev(uint8_t argc, char *argv[])
{
    const motor_port_t *port = motor_port_get();

    if (port == NULL || port->request_timed_reverse_ms == NULL) {
        motor_cli_print_fail("rev", PORT_ERR_IO);
        return 1u;
    }

    return motor_cli_handle_timed_run("rev", port->request_timed_reverse_ms,
                                      argc, argv);
}

void motor_cli_on_timed_run_done(void)
{
    char line[32];

    if (s_timed_state != MOTOR_TIMED_WAIT) {
        return;
    }

    s_timed_state = MOTOR_TIMED_IDLE;
    (void)snprintf(line, sizeof(line), "motor %s ok", s_timed_verb);
    motor_cli_timed_emit(line);
}

void motor_cli_on_timed_run_fault(void)
{
    char line[40];

    if (s_timed_state != MOTOR_TIMED_WAIT) {
        return;
    }

    s_timed_state = MOTOR_TIMED_IDLE;
    (void)snprintf(line, sizeof(line), "motor %s fault: stuck", s_timed_verb);
    motor_cli_timed_emit(line);
}

void motor_cli_test_reset_timed(void)
{
    s_timed_state = MOTOR_TIMED_IDLE;
    s_timed_verb[0] = '\0';
    s_timed_test_line[0] = '\0';
}

bool motor_cli_test_take_timed_line(char *buf, size_t len)
{
    size_t n;

    if (buf == NULL || len == 0u || s_timed_test_line[0] == '\0') {
        return false;
    }

    n = strlen(s_timed_test_line);
    if (n >= len) {
        n = len - 1u;
    }

    memcpy(buf, s_timed_test_line, n);
    buf[n] = '\0';
    s_timed_test_line[0] = '\0';
    return true;
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
