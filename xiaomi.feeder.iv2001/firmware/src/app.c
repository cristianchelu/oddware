/*
 * Application event dispatcher — spec/30-processes/app-event-loop.md
 */

#include <stdio.h>
#include <string.h>

#include "app.h"
#include "app_event.h"
#include "app_event_port.h"
#include "app_mqtt_dispatch.h"
#include "button_gesture.h"
#include "button_input.h"
#include "button_port.h"
#include "config_port.h"
#include "display_mqtt_indicator.h"
#include "display_presentation.h"
#include "display_wifi_indicator.h"
#include "mqtt_client.h"
#include "mqtt_cred.h"
#include "ota_client.h"
#include "ota_rollback.h"
#include "port_err.h"
#include "dispense_cli.h"
#include "hopper_input.h"
#include "hopper_ir_port.h"
#include "motor_cli.h"
#include "motor_jam.h"
#include "motor_port.h"
#include "weight_port.h"

#define APP_WEIGHT_SAMPLE_MS  500u

typedef enum {
    APP_DISPLAY_MODE_WEIGHT = 0,
} app_display_mode_t;

typedef enum {
    WEIGHT_BOOT_PENDING = 0,
    WEIGHT_BOOT_SETTLING,
    WEIGHT_BOOT_DONE,
} weight_boot_phase_t;

static app_display_mode_t s_display_mode = APP_DISPLAY_MODE_WEIGHT;
static weight_boot_phase_t s_weight_boot = WEIGHT_BOOT_PENDING;
static int32_t s_bowl_g;
static bool s_bowl_valid;
static uint32_t s_weight_last_sample_ms;

static void app_mqtt_session_apply(mqtt_session_phase_t phase)
{
    switch (phase) {
    case MQTT_SESSION_CONNECTING:
        display_mqtt_indicator_connecting();
        break;
    case MQTT_SESSION_CONNECTED:
        display_mqtt_indicator_connected();
        break;
    case MQTT_SESSION_ERROR:
        display_mqtt_indicator_error();
        break;
    default:
        display_mqtt_indicator_off();
        break;
    }
}

static void app_weight_sync_display_scene(void)
{
    const weight_port_t *wp = weight_port_get();
    weight_cal_status_t cal;

    if (s_display_mode != APP_DISPLAY_MODE_WEIGHT) {
        return;
    }

    cal = (wp != NULL && wp->get_cal_status != NULL) ? wp->get_cal_status()
                                                     : WEIGHT_CAL_UNCALIBRATED;

    (void)display_presentation_set_unit(DISPLAY_UNIT_GRAM);

    if (cal != WEIGHT_CAL_SUCCESS) {
        (void)display_presentation_set_digits_dash();
    } else if (s_bowl_valid) {
        uint16_t shown;

        if (s_bowl_g < 0) {
            shown = 0u;
        } else if (s_bowl_g > 999) {
            shown = 999u;
        } else {
            shown = (uint16_t)s_bowl_g;
        }
        (void)display_presentation_set_digits(shown);
    } else {
        (void)display_presentation_clear_digits();
    }
}

static void app_weight_sample(bool idle_try)
{
    const weight_port_t *wp = weight_port_get();
    int32_t grams;
    port_err_t err;

    if (wp == NULL) {
        return;
    }

    if (idle_try && wp->try_read_grams != NULL) {
        err = wp->try_read_grams(&grams);
    } else if (wp->read_grams != NULL) {
        err = wp->read_grams(&grams);
    } else {
        return;
    }

    if (err == PORT_OK) {
        s_bowl_g = grams;
        s_bowl_valid = true;
        return;
    }

    if (err == PORT_ERR_BUSY) {
        return;
    }

    if (s_bowl_valid) {
        printf("[app] weight sample lost (%s)\r\n", port_err_name(err));
    }
    s_bowl_valid = false;
}

static void app_weight_boot_first_sample(void)
{
    app_weight_sample(false);
    app_weight_sync_display_scene();
}

static void app_weight_idle_tick(void)
{
    app_weight_sample(true);
    app_weight_sync_display_scene();
}

static void app_weight_idle_on_display_tick(uint32_t now_ms)
{
    if (s_weight_boot != WEIGHT_BOOT_DONE) {
        return;
    }

    if (s_weight_last_sample_ms != 0u &&
        (now_ms - s_weight_last_sample_ms) < APP_WEIGHT_SAMPLE_MS) {
        return;
    }

    s_weight_last_sample_ms = now_ms;
    app_weight_idle_tick();
}

static void app_weight_boot_advance(void)
{
    const weight_port_t *wp = weight_port_get();

    if (s_weight_boot == WEIGHT_BOOT_PENDING) {
        if (wp != NULL && wp->boot_begin != NULL) {
            (void)wp->boot_begin();
        }
        s_weight_boot = WEIGHT_BOOT_SETTLING;
        return;
    }

    if (s_weight_boot == WEIGHT_BOOT_SETTLING) {
        port_err_t err = PORT_ERR_IO;

        if (wp != NULL && wp->boot_poll != NULL) {
            err = wp->boot_poll();
        }

        if (err == PORT_ERR_BUSY) {
            return;
        }

        if (err == PORT_OK) {
            app_weight_boot_first_sample();
            s_weight_last_sample_ms = 0u;
        }

        s_weight_boot = WEIGHT_BOOT_DONE;
    }
}

static void app_weight_boot_arm(void)
{
    s_weight_boot = WEIGHT_BOOT_PENDING;
    s_bowl_valid = false;
}

static const char *app_button_press_label(button_id_t id)
{
    switch (id) {
    case BUTTON_ID_POWER:
        return "power";
    case BUTTON_ID_RESET:
        return "reset";
    case BUTTON_ID_DISPENSE:
        return "dispense";
    default:
        return "unknown";
    }
}

static char s_test_btn_log[48];

static void app_button_log_line(const char *line)
{
    printf("%s\r\n", line);
    (void)snprintf(s_test_btn_log, sizeof(s_test_btn_log), "%s", line);
}

static void app_button_log_press(button_id_t id)
{
    char line[48];

    (void)snprintf(line,
                   sizeof(line),
                   "[btn] %s pressed",
                   app_button_press_label(id));
    app_button_log_line(line);
}

static void app_button_log_gesture(const button_gesture_event_t *ev)
{
    char line[48];
    const char *label;
    const char *kind;

    if (ev == NULL) {
        return;
    }

    if (ev->kind == BUTTON_GESTURE_CHILD_LOCK_TOGGLE) {
        app_button_log_line("[btn] child_lock toggle");
        return;
    }

    label = app_button_press_label(ev->id);
    kind = (ev->kind == BUTTON_GESTURE_LONG) ? "long" : "short";
    (void)snprintf(line, sizeof(line), "[btn] %s %s", label, kind);
    app_button_log_line(line);
}

void app_test_clear_btn_log(void)
{
    s_test_btn_log[0] = '\0';
}

bool app_test_take_btn_log(char *buf, size_t len)
{
    size_t n;

    if (buf == NULL || len == 0u || s_test_btn_log[0] == '\0') {
        return false;
    }

    n = strlen(s_test_btn_log);
    if (n >= len) {
        n = len - 1u;
    }

    memcpy(buf, s_test_btn_log, n);
    buf[n] = '\0';
    s_test_btn_log[0] = '\0';
    return true;
}

static void app_button_drain(void)
{
    button_transition_t tr;
    button_gesture_event_t gesture;

    while (button_input_pop_transition(&tr)) {
        if (tr.edge == BUTTON_EDGE_DOWN) {
            app_button_log_press(tr.id);
        }
        button_gesture_on_transition(&tr);
    }

    while (button_gesture_pop(&gesture)) {
        app_button_log_gesture(&gesture);
    }
}

static void app_button_poll(uint32_t now_ms)
{
    button_input_poll(now_ms);
    button_gesture_step(now_ms);
    app_button_drain();
}

void app_dispatch(const app_event_t *ev)
{
    if (ev == NULL) {
        return;
    }

    switch (ev->type) {
    case EVT_APP_BOOT:
        s_display_mode = APP_DISPLAY_MODE_WEIGHT;
        display_presentation_reset();
        app_weight_boot_arm();
        break;

    case EVT_WIFI_STA_CONNECTING:
        display_wifi_indicator_connecting();
        break;

    case EVT_WIFI_STA_READY:
        display_wifi_indicator_connected();
        mqtt_client_notify_wifi_ready();
        if (mqtt_cred_is_stored(config_port_get())) {
            (void)mqtt_client_request_connect();
        }
        break;

    case EVT_WIFI_STA_FAILED:
        display_wifi_indicator_off();
        break;

    case EVT_WIFI_STA_AP_MODE:
        display_wifi_indicator_ap_mode();
        break;

    case EVT_MQTT_SESSION:
        app_mqtt_session_apply(ev->u.mqtt_session.phase);
        break;

    case EVT_MQTT_CONNECTED:
        ota_client_on_mqtt_connected();
        break;

    case EVT_MQTT_MESSAGE:
        app_mqtt_dispatch(ev->u.mqtt_message.topic,
                          ev->u.mqtt_message.payload,
                          ev->u.mqtt_message.len,
                          mqtt_client_device_id());
        break;

    case EVT_DISPLAY_TICK:
        app_weight_idle_on_display_tick(ev->u.display_tick.now_ms);
        (void)display_presentation_tick(ev->u.display_tick.now_ms);
        app_button_poll(ev->u.display_tick.now_ms);
        hopper_input_poll(ev->u.display_tick.now_ms);
        break;

    case EVT_BUTTON_IRQ:
        button_input_notify_irq(ev->u.button_irq.now_ms);
        app_button_poll(ev->u.button_irq.now_ms);
        break;

    case EVT_TIMER_TICK:
        (void)ota_rollback_poll_ms();
        app_weight_boot_advance();
        break;

    case EVT_DISPENSE_START: {
        const motor_port_t *motor = motor_port_get();
        port_err_t err = PORT_ERR_IO;

        if (motor != NULL && motor->request_burst != NULL) {
            err = motor->request_burst(MOTOR_BURST_PULSE_DEFAULT,
                                       MOTOR_BURST_TIMEOUT_MS);
        }

        if (err != PORT_OK) {
            printf("dispense busy\r\n");
            dispense_cli_cancel_wait();
        }
        break;
    }

    case EVT_BURST_DONE:
        if (dispense_cli_on_burst_done()) {
            hopper_input_notify_dispense_complete();
        }
        break;

    case EVT_MOTOR_FAULT:
        if (dispense_cli_on_motor_fault()) {
            hopper_input_notify_dispense_complete();
        }
        motor_cli_on_park_fault();
        break;

    case EVT_PARK_DONE:
        motor_cli_on_park_done();
        break;

    default:
        break;
    }
}

void app_test_reset(void)
{
    s_display_mode = APP_DISPLAY_MODE_WEIGHT;
    s_weight_boot = WEIGHT_BOOT_PENDING;
    s_bowl_g = 0;
    s_bowl_valid = false;
    s_weight_last_sample_ms = 0u;
    button_input_init(button_port_get());
    hopper_input_init(hopper_ir_port_get());
    button_gesture_reset();
    app_event_port_init();
}

bool app_step(void)
{
    app_event_t ev;
    bool handled = false;

    while (app_event_try_receive(&ev)) {
        app_dispatch(&ev);
        app_event_release(&ev);
        handled = true;
    }

    return handled;
}
