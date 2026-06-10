/*
 * Display bench CLI logic — spec/30-processes/uart-console.md
 */

#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "display_boot.h"
#include "display_cli.h"
#include "display_port.h"

port_err_t display_cli_parse_hex_byte(const char *text, uint8_t *out)
{
    char *end = NULL;
    unsigned long val;

    if (text == NULL || out == NULL || text[0] == '\0') {
        return PORT_ERR_INVALID_ARG;
    }

    val = strtoul(text, &end, 16);
    if (end == text || *end != '\0' || val > 0xFFu) {
        return PORT_ERR_INVALID_ARG;
    }

    *out = (uint8_t)val;
    return PORT_OK;
}

__attribute__((weak)) void display_cli_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

port_err_t display_cli_run_test(void)
{
    const display_port_t *dp = display_port_get();
    port_err_t err;

    if (dp == NULL) {
        return PORT_ERR_IO;
    }

    err = dp->power_on();
    if (err != PORT_OK) {
        return err;
    }

    err = dp->show_fill(0xFFu);
    if (err != PORT_OK) {
        (void)dp->power_off();
        return err;
    }

    display_cli_delay_ms(DISPLAY_BOOT_LIGHT_TEST_MS);

    err = dp->blank();
    if (err != PORT_OK) {
        (void)dp->power_off();
        return err;
    }

    return dp->power_off();
}

port_err_t display_cli_run_fill(uint8_t segment_byte)
{
    const display_port_t *dp = display_port_get();
    port_err_t err;

    if (dp == NULL) {
        return PORT_ERR_IO;
    }

    err = dp->power_on();
    if (err != PORT_OK) {
        return err;
    }

    return dp->show_fill(segment_byte);
}

port_err_t display_cli_run_off(void)
{
    const display_port_t *dp = display_port_get();

    if (dp == NULL) {
        return PORT_ERR_IO;
    }

    return dp->power_off();
}
