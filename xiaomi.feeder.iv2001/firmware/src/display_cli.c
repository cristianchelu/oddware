/*
 * Display bench CLI logic — spec/30-processes/uart-console.md
 */

#include <stdio.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "app_log.h"
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

port_err_t display_cli_parse_number(const char *text, uint16_t *out)
{
    char *end = NULL;
    unsigned long val;

    if (text == NULL || out == NULL || text[0] == '\0') {
        return PORT_ERR_INVALID_ARG;
    }

    val = strtoul(text, &end, 10);
    if (end == text || *end != '\0' || val > 999u) {
        return PORT_ERR_INVALID_ARG;
    }

    *out = (uint16_t)val;
    return PORT_OK;
}

port_err_t display_cli_parse_blink_ms(const char *text, uint16_t *out)
{
    char *end = NULL;
    unsigned long val;

    if (text == NULL || out == NULL || text[0] == '\0') {
        return PORT_ERR_INVALID_ARG;
    }

    val = strtoul(text, &end, 10);
    if (end == text || *end != '\0' || val > DISPLAY_PRESENTATION_BLINK_MAX_MS) {
        return PORT_ERR_INVALID_ARG;
    }

    *out = (uint16_t)val;
    return PORT_OK;
}

__attribute__((weak)) void display_cli_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

void display_cli_print_fail(const char *what, port_err_t err)
{
    app_log_info("cli", "display %s failed (%s)", what, port_err_name(err));
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
    return display_presentation_power_off();
}

port_err_t display_cli_run_number(uint16_t value, display_unit_t unit)
{
    port_err_t err;

    err = display_presentation_set_digits(value);
    if (err != PORT_OK) {
        return err;
    }

    err = display_presentation_set_unit(unit);
    if (err != PORT_OK) {
        return err;
    }

    return display_presentation_refresh();
}

port_err_t display_cli_run_icon_set(display_icon_t icon, bool on)
{
    port_err_t err = display_presentation_icon_set(icon, on);

    if (err != PORT_OK) {
        return err;
    }

    return display_presentation_refresh();
}

port_err_t display_cli_run_icon_blink(display_icon_t icon,
                                    uint16_t on_ms,
                                    uint16_t off_ms)
{
    port_err_t err = display_presentation_icon_blink(icon, on_ms, off_ms);

    if (err != PORT_OK) {
        return err;
    }

    (void)display_presentation_tick(0u);
    return display_presentation_refresh();
}

port_err_t display_cli_run_icon_steady(display_icon_t icon)
{
    port_err_t err = display_presentation_icon_blink_stop(icon);

    if (err != PORT_OK) {
        return err;
    }

    return display_presentation_refresh();
}

port_err_t display_cli_run_anim(display_builtin_anim_t id, bool loop)
{
    port_err_t err = display_presentation_play_builtin(id, loop);

    if (err != PORT_OK) {
        return err;
    }

    (void)display_presentation_tick(0u);
    return display_presentation_refresh();
}

port_err_t display_cli_run_anim_stop(void)
{
    port_err_t err = display_presentation_stop_animation();

    if (err != PORT_OK) {
        return err;
    }

    return display_presentation_refresh();
}

port_err_t display_cli_run_brightness(uint8_t level)
{
    port_err_t err = display_presentation_set_brightness(level);

    if (err != PORT_OK) {
        return err;
    }

    return display_presentation_refresh();
}
