/*
 * Dispense UART CLI — spec/30-processes/uart-console.md § dispense
 */

#include <stdio.h>
#include <string.h>

#include "app_log.h"
#include "dispense.h"
#include "dispense_cli.h"
#include "motor_cli.h"

typedef enum {
    DISPENSE_CLI_IDLE = 0,
    DISPENSE_CLI_WAIT_JOB,
} dispense_cli_state_t;

static dispense_cli_state_t s_state = DISPENSE_CLI_IDLE;
static char s_test_line[48];

static void dispense_cli_emit(const char *line)
{
    app_log_info("cli", "%s", line);
    (void)snprintf(s_test_line, sizeof(s_test_line), "%s", line);
}

port_err_t dispense_cli_parse_portions(const char *text, uint8_t *out)
{
    uint32_t value = 0u;
    port_err_t err;

    if (out == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = motor_cli_parse_duration_ms(text, &value);
    if (err != PORT_OK) {
        return err;
    }

    if (value > DISPENSE_PORTIONS_MAX) {
        return PORT_ERR_INVALID_ARG;
    }

    *out = (uint8_t)value;
    return PORT_OK;
}

static uint8_t dispense_cli_submit(uint8_t portions)
{
    dispense_submit_result_t result;

    result = dispense_submit_portions(portions);
    if (result == DISPENSE_SUBMIT_INVALID) {
        dispense_cli_emit("dispense usage: portions <1-15>");
        return 1u;
    }

    if (result != DISPENSE_SUBMIT_OK) {
        return 1u;
    }

    s_state = DISPENSE_CLI_WAIT_JOB;
    return 0u;
}

uint8_t dispense_cli_handle_default(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;
    return dispense_cli_submit(1u);
}

uint8_t dispense_cli_handle_portions(uint8_t argc, char *argv[])
{
    uint8_t portions;
    port_err_t err;

    if (argc < 1u || argv == NULL || argv[0] == NULL) {
        dispense_cli_emit("dispense usage: portions <1-15>");
        return 1u;
    }

    err = dispense_cli_parse_portions(argv[0], &portions);
    if (err != PORT_OK) {
        dispense_cli_emit("dispense usage: portions <1-15>");
        return 1u;
    }

    return dispense_cli_submit(portions);
}

bool dispense_cli_on_job_done(void)
{
    if (s_state != DISPENSE_CLI_WAIT_JOB) {
        return false;
    }

    s_state = DISPENSE_CLI_IDLE;
    dispense_cli_emit("dispense done");
    return true;
}

bool dispense_cli_on_job_fault(void)
{
    if (s_state != DISPENSE_CLI_WAIT_JOB) {
        return false;
    }

    s_state = DISPENSE_CLI_IDLE;
    dispense_cli_emit("dispense fault: stuck");
    return true;
}

void dispense_cli_cancel_wait(void)
{
    s_state = DISPENSE_CLI_IDLE;
}

void dispense_cli_test_reset(void)
{
    s_state = DISPENSE_CLI_IDLE;
    s_test_line[0] = '\0';
}

bool dispense_cli_test_take_line(char *buf, size_t len)
{
    size_t n;

    if (buf == NULL || len == 0u || s_test_line[0] == '\0') {
        return false;
    }

    n = strlen(s_test_line);
    if (n >= len) {
        n = len - 1u;
    }

    memcpy(buf, s_test_line, n);
    buf[n] = '\0';
    s_test_line[0] = '\0';
    return true;
}
