/*
 * Application event dispatcher — spec/30-processes/app-event-loop.md
 */

#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "app_log.h"
#include "app.h"
#include "app_event.h"
#include "app_event_port.h"
#include "app_mqtt_dispatch.h"
#include "auto_tare.h"
#include "bowl_mass_present.h"
#include "bowl_error.h"
#include "button_gesture.h"
#include "button_input.h"
#include "button_port.h"
#include "config_port.h"
#include "display_mqtt_indicator.h"
#include "display_child_lock_indicator.h"
#include "display_bowl_error_indicator.h"
#include "display_presentation.h"
#include "mqtt_state.h"
#include "mqtt_bowl_weight.h"
#include "mqtt_mains.h"
#include "display_wifi_indicator.h"
#include "FreeRTOS.h"
#include "mqtt_client.h"
#include "task.h"

#if REMOTE_CLI_ENABLE
#include "remote_cli.h"
#endif
#if WEB_UI_ENABLE
#include "web_ui.h"
#endif
#include "mqtt_cred.h"
#include "ota_client.h"
#include "ota_slot_health.h"
#include "port_err.h"
#include "time_sync.h"
#include "schedule.h"
#include "dispense.h"
#include "feeder_runtime.h"
#include "dispense_cli.h"
#include "feed_config.h"
#include "hopper_input.h"
#include "hopper_ir_port.h"
#include "hopper_level.h"
#include "mqtt_hopper.h"
#include "power_source_input.h"
#include "power_source_port.h"
#include "battery_monitor.h"
#include "motor_cli.h"
#include "motor_port.h"
#include "weigh_product.h"
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
static weight_dg_t s_bowl_dg;
static bool s_bowl_valid;
static uint32_t s_weight_last_sample_ms;
static bool s_weight_resample_after_dispense;
static bool s_bowl_missing_known;
static bool s_bowl_missing;

static void app_weight_log_bowl_presence(bowl_error_kind_t bowl_err)
{
    bool missing = (bowl_err == BOWL_ERROR_BOWL_MISSING);

    if (!s_bowl_missing_known) {
        s_bowl_missing_known = true;
        s_bowl_missing = missing;
        if (missing) {
            app_log_info("app", "bowl missing");
        }
        return;
    }

    if (missing == s_bowl_missing) {
        return;
    }

    s_bowl_missing = missing;
    app_log_info("app", missing ? "bowl missing" : "bowl present");
}

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

static void app_weight_sync_display_scene(bool force_mqtt_bowl_weight)
{
    const weight_port_t *wp = weight_port_get();
    weight_cal_status_t cal;
    bowl_error_kind_t bowl_err;
    bowl_mass_status_t mass_st;
    weight_dg_t present_dg;
    uint16_t shown;
    bowl_display_digits_t digits;

    if (s_display_mode != APP_DISPLAY_MODE_WEIGHT) {
        return;
    }

    cal = (wp != NULL && wp->get_cal_status != NULL) ? wp->get_cal_status()
                                                     : WEIGHT_CAL_UNCALIBRATED;

    bowl_err = bowl_error_eval(cal, s_bowl_valid, s_bowl_dg);
    display_bowl_error_indicator_sync(bowl_err);

    mqtt_state_sync(bowl_error_is_active(bowl_err));

    {
        weight_dg_t presented_dg = auto_tare_present_dg(s_bowl_dg, s_bowl_valid);

        mass_st = bowl_mass_present_dg(cal, s_bowl_valid, presented_dg, &present_dg);
        mqtt_bowl_weight_sync(mass_st, present_dg, force_mqtt_bowl_weight);

        (void)display_presentation_set_unit(DISPLAY_UNIT_GRAM);

        digits = bowl_mass_display_digits(cal, s_bowl_valid, presented_dg, &shown);
    }
    app_weight_log_bowl_presence(bowl_err);
    switch (digits) {
    case BOWL_DISPLAY_DASH:
        (void)display_presentation_set_digits_dash();
        break;
    case BOWL_DISPLAY_UNDERFLOW:
        (void)display_presentation_set_digits_underflow();
        break;
    case BOWL_DISPLAY_BLANK:
        (void)display_presentation_clear_digits();
        break;
    case BOWL_DISPLAY_GRAMS:
        (void)display_presentation_set_digits(shown);
        break;
    default:
        (void)display_presentation_clear_digits();
        break;
    }
}

static void app_weight_sample(bool idle_try)
{
    const weight_port_t *wp = weight_port_get();
    weight_dg_t dg;
    port_err_t err;

    if (wp == NULL) {
        return;
    }

    if (idle_try && wp->try_read_dg != NULL) {
        err = wp->try_read_dg(&dg);
    } else if (wp->read_dg != NULL) {
        err = wp->read_dg(&dg);
    } else {
        return;
    }

    if (err == PORT_OK) {
        s_bowl_dg = dg;
        s_bowl_valid = true;
        return;
    }

    if (err == PORT_ERR_BUSY) {
        return;
    }

    if (s_bowl_valid) {
        app_log_info("app", "weight sample lost (%s)", port_err_name(err));
    }
    s_bowl_valid = false;
}

static void app_weight_boot_first_sample(void)
{
    app_weight_sample(false);
    app_weight_sync_display_scene(false);
}

static void app_weight_idle_tick(void)
{
    const weight_port_t *wp = weight_port_get();
    weight_cal_status_t cal;
    bowl_error_kind_t bowl_err;

    app_weight_sample(true);

    cal = (wp != NULL && wp->get_cal_status != NULL) ? wp->get_cal_status()
                                                     : WEIGHT_CAL_UNCALIBRATED;
    bowl_err = bowl_error_eval(cal, s_bowl_valid, s_bowl_dg);
    auto_tare_sync_bowl_error(bowl_err);

    auto_tare_idle_sample(s_bowl_dg, s_bowl_valid);
    app_weight_sync_display_scene(false);
}

void app_weight_notify_dispense_complete(void)
{
    if (s_weight_boot != WEIGHT_BOOT_DONE) {
        return;
    }

    s_weight_resample_after_dispense = true;
}

static void app_weight_resample_after_dispense_if_needed(void)
{
    if (!s_weight_resample_after_dispense) {
        return;
    }

    s_weight_resample_after_dispense = false;
    app_weight_sample(false);
    app_weight_sync_display_scene(true);
}

static void app_weight_idle_on_display_tick(uint32_t now_ms)
{
    if (s_weight_boot != WEIGHT_BOOT_DONE) {
        return;
    }

    if (feeder_runtime_dispense_active()) {
        return;
    }

    if (s_weight_last_sample_ms != 0u &&
        (now_ms - s_weight_last_sample_ms) < APP_WEIGHT_SAMPLE_MS) {
        return;
    }

    s_weight_last_sample_ms = now_ms;
    app_weight_idle_tick();
}

static void app_power_mqtt_sync(void)
{
    power_source_transition_t tr;

    while (power_source_input_pop_transition(&tr)) {
        mqtt_mains_sync(tr.edge == POWER_SOURCE_EDGE_MAINS);
        battery_monitor_force_sample();
        battery_monitor_poll(tr.at_ms);
    }
}

static void app_hopper_mqtt_sync(void)
{
    hopper_level_state_transition_t tr;

    while (hopper_level_pop_transition(&tr)) {
        mqtt_hopper_sync(tr.level);
    }
}

bool app_bowl_dg_snapshot(uint32_t now_ms, app_bowl_dg_snapshot_t *out)
{
    if (out == NULL || s_weight_boot != WEIGHT_BOOT_DONE) {
        return false;
    }

    out->valid = s_bowl_valid;
    out->dg = s_bowl_dg;
    if (!s_bowl_valid || s_weight_last_sample_ms == 0u) {
        out->sample_age_ms = UINT32_MAX;
    } else if (now_ms >= s_weight_last_sample_ms) {
        out->sample_age_ms = now_ms - s_weight_last_sample_ms;
    } else {
        out->sample_age_ms = 0u;
    }

    return true;
}

void app_bowl_dg_notify_read(weight_dg_t dg, bool valid, uint32_t now_ms)
{
    s_bowl_dg = dg;
    s_bowl_valid = valid;
    s_weight_last_sample_ms = now_ms;
    app_weight_sync_display_scene(true);
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
    app_log_info("app", "%s", line);
    (void)snprintf(s_test_btn_log, sizeof(s_test_btn_log), "%s", line);
}

static void app_button_log_press(button_id_t id)
{
    char line[48];

    (void)snprintf(line,
                   sizeof(line),
                   "btn %s pressed",
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
        (void)snprintf(line, sizeof(line), "btn child_lock toggle");
        app_log_debug("app", "%s", line);
        (void)snprintf(s_test_btn_log, sizeof(s_test_btn_log), "%s", line);
        return;
    }

    label = app_button_press_label(ev->id);
    kind = (ev->kind == BUTTON_GESTURE_LONG) ? "long" : "short";
    (void)snprintf(line, sizeof(line), "btn %s %s", label, kind);
    app_log_debug("app", "%s", line);
    (void)snprintf(s_test_btn_log, sizeof(s_test_btn_log), "%s", line);
}

static void app_child_lock_sync_display(bool locked)
{
    if (display_child_lock_indicator_feedback_active()) {
        return;
    }

    (void)display_presentation_icon_blink_stop(DISPLAY_ICON_CHILD_LOCK);
    (void)display_presentation_icon_set(DISPLAY_ICON_CHILD_LOCK, locked);
}

static void app_button_handle_gesture(const button_gesture_event_t *ev)
{
    if (ev == NULL) {
        return;
    }

    if (ev->kind == BUTTON_GESTURE_CHILD_LOCK_TOGGLE) {
        bool locked;

        display_child_lock_indicator_cancel();
        locked = feed_config_child_lock_toggle();
        app_child_lock_sync_display(locked);
        app_log_info("app", "child_lock %s", locked ? "on" : "off");
        return;
    }

    if (feed_config_child_lock_is_active()) {
        display_child_lock_indicator_blocked_feedback(true, ev->at_ms);
        app_log_info("app", "child_lock blocked");
        return;
    }

    if (ev->id == BUTTON_ID_DISPENSE && ev->kind == BUTTON_GESTURE_SHORT) {
        (void)dispense_submit_portions(1u, DISPENSE_SOURCE_BUTTON);
    }
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

static void app_button_apply_transitions(void)
{
    button_transition_t tr;

    while (button_input_pop_transition(&tr)) {
        if (tr.edge == BUTTON_EDGE_DOWN) {
            app_button_log_press(tr.id);
        }
        button_gesture_on_transition(&tr);
    }
}

static void app_button_drain_gestures(void)
{
    button_gesture_event_t gesture;
    bool suppress_combo_partners = false;

    while (button_gesture_pop(&gesture)) {
        if (gesture.kind == BUTTON_GESTURE_CHILD_LOCK_TOGGLE) {
            suppress_combo_partners = true;
        } else if (suppress_combo_partners &&
                   button_gesture_is_combo_partner_gesture(&gesture)) {
            continue;
        }

        app_button_log_gesture(&gesture);
        app_button_handle_gesture(&gesture);
    }
}

static void app_button_poll(uint32_t now_ms)
{
    button_input_poll(now_ms);
    app_button_apply_transitions();
    button_gesture_step(now_ms);
    app_button_drain_gestures();
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
        app_child_lock_sync_display(feed_config_child_lock_is_active());
        auto_tare_init();
        feeder_runtime_init();
        app_weight_boot_arm();
        break;

    case EVT_WIFI_STA_CONNECTING:
        display_wifi_indicator_connecting();
        break;

    case EVT_WIFI_STA_READY:
        display_wifi_indicator_connected();
        time_sync_on_wifi_ready();
        mqtt_client_notify_wifi_ready();
#if REMOTE_CLI_ENABLE
        remote_cli_start();
#endif
#if WEB_UI_ENABLE
        web_ui_start();
#endif
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
        app_mqtt_on_connected();
        break;

    case EVT_MQTT_MESSAGE:
        app_mqtt_dispatch(ev->u.mqtt_message.topic,
                          ev->u.mqtt_message.payload,
                          ev->u.mqtt_message.len,
                          mqtt_client_device_id());
        break;

    case EVT_DISPLAY_TICK:
        dispense_poll(ev->u.display_tick.now_ms);
        app_button_poll(ev->u.display_tick.now_ms);
        if (!display_child_lock_indicator_feedback_active()) {
            app_weight_resample_after_dispense_if_needed();
            app_weight_idle_on_display_tick(ev->u.display_tick.now_ms);
        }
        (void)display_presentation_tick(ev->u.display_tick.now_ms);
        if (display_child_lock_indicator_poll(ev->u.display_tick.now_ms)) {
            app_child_lock_sync_display(feed_config_child_lock_is_active());
            app_weight_sync_display_scene(false);
            (void)display_presentation_refresh();
        }
        power_source_input_poll(ev->u.display_tick.now_ms);
        {
            bool hopper_bg = power_source_input_is_valid() &&
                             power_source_input_get() == POWER_SOURCE_MAINS;

            hopper_input_poll(ev->u.display_tick.now_ms, hopper_bg);
        }
        hopper_level_poll();
        app_hopper_mqtt_sync();
        app_power_mqtt_sync();
        break;

    case EVT_BUTTON_IRQ:
        button_input_notify_irq(ev->u.button_irq.now_ms);
        power_source_input_notify_irq(ev->u.button_irq.now_ms);
        app_button_poll(ev->u.button_irq.now_ms);
        power_source_input_poll(ev->u.button_irq.now_ms);
        app_power_mqtt_sync();
        break;

    case EVT_TIMER_TICK:
        {
            uint32_t now_ms =
                (uint32_t)(xTaskGetTickCount() * (TickType_t)portTICK_PERIOD_MS);

            dispense_poll(now_ms);
            schedule_poll(now_ms);
            (void)ota_slot_health_poll_ms();
            time_sync_poll(now_ms);
            app_weight_boot_advance();
            battery_monitor_poll(now_ms);
        }
        break;

    case EVT_DISPENSE_REQUEST:
        dispense_start_from_request(&ev->u.dispense_request);
        break;

    case EVT_BURST_DONE:
        (void)dispense_on_burst_done();
        break;

    case EVT_MOTOR_FAULT:
        if (!dispense_on_motor_fault()) {
            motor_cli_on_park_fault();
            motor_cli_on_timed_run_fault();
        }
        break;

    case EVT_PARK_DONE:
        motor_cli_on_park_done();
        break;

    case EVT_TIMED_RUN_DONE:
        motor_cli_on_timed_run_done();
        break;

    default:
        break;
    }
}

#define APP_EVENT_DRAIN_MAX  16u

static bool app_event_defer_display_tick(app_event_type_t type)
{
    return type == EVT_DISPLAY_TICK;
}

static void app_dispatch_event_batch(app_event_t *batch, size_t count)
{
    size_t i;
    bool display_dispatched = false;

    for (i = 0u; i < count; i++) {
        if (!app_event_defer_display_tick(batch[i].type)) {
            app_dispatch(&batch[i]);
            app_event_release(&batch[i]);
        }
    }

    for (i = 0u; i < count; i++) {
        if (!app_event_defer_display_tick(batch[i].type)) {
            continue;
        }

        if (!display_dispatched) {
            app_dispatch(&batch[i]);
            display_dispatched = true;
        }
        app_event_release(&batch[i]);
    }
}

static size_t app_collect_queued_events(app_event_t *batch, size_t max, bool have_first,
                                        const app_event_t *first)
{
    size_t count = 0u;
    app_event_t ev;

    if (have_first && first != NULL && count < max) {
        batch[count++] = *first;
    }

    while (count < max && app_event_try_receive(&ev)) {
        batch[count++] = ev;
    }

    return count;
}

void app_drain_queued_events(void)
{
    app_event_t batch[APP_EVENT_DRAIN_MAX];
    size_t count;

    count = app_collect_queued_events(batch, APP_EVENT_DRAIN_MAX, false, NULL);
    if (count > 0u) {
        app_dispatch_event_batch(batch, count);
    }
}

void app_process_received_event(const app_event_t *ev)
{
    app_event_t batch[APP_EVENT_DRAIN_MAX];
    size_t count;

    count = app_collect_queued_events(batch, APP_EVENT_DRAIN_MAX, true, ev);
    app_dispatch_event_batch(batch, count);
}

void app_test_reset(void)
{
    s_display_mode = APP_DISPLAY_MODE_WEIGHT;
    s_weight_boot = WEIGHT_BOOT_PENDING;
    s_bowl_dg = 0;
    s_bowl_valid = false;
    s_weight_last_sample_ms = 0u;
    s_weight_resample_after_dispense = false;
    s_bowl_missing_known = false;
    s_bowl_missing = false;
    feeder_runtime_test_reset();
    auto_tare_test_reset();
    button_input_init(button_port_get());
    hopper_input_init(hopper_ir_port_get());
    hopper_level_init();
    power_source_input_init(power_source_port_get());
    button_gesture_reset();
    dispense_test_reset();
    dispense_cli_test_reset();
    app_event_port_init();
}

void app_test_reset_bowl_presence_log(void)
{
    s_bowl_missing_known = false;
    s_bowl_missing = false;
}

void app_test_finish_weight_boot(void)
{
    s_weight_boot = WEIGHT_BOOT_DONE;
}

bool app_step(void)
{
    app_event_t batch[APP_EVENT_DRAIN_MAX];
    size_t count;

    count = app_collect_queued_events(batch, APP_EVENT_DRAIN_MAX, false, NULL);
    if (count == 0u) {
        return false;
    }

    app_dispatch_event_batch(batch, count);
    return true;
}
