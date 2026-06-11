/*
 * Dispense UART CLI — spec/30-processes/uart-console.md § dispense
 */

#include <stdio.h>
#include <string.h>

#include "app_event.h"
#include "dispense_cli.h"
#include "motor_port.h"

typedef enum {
    DISPENSE_CLI_IDLE = 0,
    DISPENSE_CLI_WAIT_BURST,
} dispense_cli_state_t;

static dispense_cli_state_t s_state = DISPENSE_CLI_IDLE;
static char s_test_line[48];

static void dispense_cli_emit(const char *line)
{
    printf("%s\r\n", line);
    (void)snprintf(s_test_line, sizeof(s_test_line), "%s", line);
}

uint8_t dispense_cli_handle(uint8_t argc, char *argv[])
{
    app_event_t ev;
    const motor_port_t *motor = motor_port_get();

    (void)argc;
    (void)argv;

    if (motor != NULL && motor->is_active != NULL && motor->is_active()) {
        dispense_cli_emit("dispense busy");
        return 1u;
    }

    if (s_state != DISPENSE_CLI_IDLE) {
        dispense_cli_emit("dispense busy");
        return 1u;
    }

    ev.type = EVT_DISPENSE_START;
    if (!app_event_post(&ev)) {
        dispense_cli_emit("dispense busy");
        return 1u;
    }

    s_state = DISPENSE_CLI_WAIT_BURST;
    dispense_cli_emit("dispense started");
    return 0u;
}

bool dispense_cli_on_burst_done(void)
{
    if (s_state != DISPENSE_CLI_WAIT_BURST) {
        return false;
    }

    s_state = DISPENSE_CLI_IDLE;
    dispense_cli_emit("dispense done");
    return true;
}

bool dispense_cli_on_motor_fault(void)
{
    if (s_state != DISPENSE_CLI_WAIT_BURST) {
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
