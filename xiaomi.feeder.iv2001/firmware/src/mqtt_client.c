/*
 * MQTT client orchestration — spec/30-processes/mqtt-protocol.md
 */

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "wifi_api.h"

#include "task_def.h"

#include "app_log.h"
#include "boot_bank_target.h"
#include "app_event.h"
#include "app_event_port.h"
#include "config_port.h"
#include "mqtt_adapter.h"
#include "mqtt_backoff.h"
#include "mqtt_client.h"
#include "mqtt_cred.h"
#include "mqtt_port.h"
#include "mqtt_route.h"
#include "mqtt_topics.h"
#include "ota_client.h"
#include "wifi_port.h"

#define MQTT_OFFLINE_PAYLOAD       "{\"online\": false}"
#define MQTT_CMD_WILDCARD          "cmd/#"
#define MQTT_CONNECT_WORKER_STACK  4096u
#define MQTT_CONNECT_POLL_MS       50u

#ifndef APP_TASK_PRIO
#define APP_TASK_PRIO              2
#endif

#define MQTT_CONNECT_WORKER_PRIO   ((APP_TASK_PRIO > 0) ? (APP_TASK_PRIO - 1) : 1)

static TaskHandle_t s_mqtt_task;
static TaskHandle_t s_connect_worker;
static volatile bool s_connect_busy;
static volatile bool s_connect_worker_running;
static volatile bool s_connect_worker_done;
static volatile port_err_t s_connect_worker_result;
static bool s_unit_test_mode;
static volatile bool s_wifi_ready;
static volatile bool s_connect_pending;
static volatile bool s_connect_armed;
static volatile bool s_disconnect_pending;
static volatile bool s_suspended;
static bool s_log_connect_failures;
static mqtt_backoff_t s_backoff;
static TickType_t s_reconnect_at;
static char s_device_id[MQTT_DEVICE_ID_MAX_LEN + 1];

static mqtt_session_phase_t s_session_phase = MQTT_SESSION_OFF;

static mqtt_session_phase_t mqtt_client_derive_session_phase(const mqtt_port_t *mqtt)
{
    if (s_suspended || !s_connect_armed || !mqtt_client_wifi_is_ready()) {
        return MQTT_SESSION_OFF;
    }

    if (mqtt != NULL && mqtt->is_connected()) {
        return MQTT_SESSION_CONNECTED;
    }

    if (s_reconnect_at != 0 && xTaskGetTickCount() < s_reconnect_at) {
        return MQTT_SESSION_ERROR;
    }

    if (s_connect_busy || s_connect_pending) {
        return MQTT_SESSION_CONNECTING;
    }

    return MQTT_SESSION_OFF;
}

static void mqtt_client_sync_session(const mqtt_port_t *mqtt)
{
    mqtt_session_phase_t phase = mqtt_client_derive_session_phase(mqtt);
    app_event_t ev;

    if (phase == s_session_phase) {
        return;
    }

    s_session_phase = phase;
    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_MQTT_SESSION;
    ev.u.mqtt_session.phase = phase;
    (void)app_event_post(&ev);
}

static void mqtt_client_get_mac_hex(char *buf, size_t len)
{
    uint8_t mac[6];
    int written;
    int i;

    if (buf == NULL || len < 13) {
        if (buf != NULL && len > 0) {
            buf[0] = '\0';
        }
        return;
    }

    if (wifi_config_get_mac_address(WIFI_PORT_STA, mac) < 0) {
        buf[0] = '\0';
        return;
    }

    written = 0;
    for (i = 0; i < 6; i++) {
        written += snprintf(buf + written, len - (size_t)written, "%02x", mac[i]);
    }
}

static bool mqtt_client_wifi_has_ip(void)
{
    const wifi_port_t *wifi = wifi_port_get();
    char ip[20];

    if (!wifi->is_connected()) {
        return false;
    }

    return wifi->get_ip(ip, sizeof(ip)) == PORT_OK;
}

static port_err_t mqtt_client_publish_online(const mqtt_port_t *mqtt)
{
    char topic[96];
    char payload[64];
    boot_bank_t active;
    int written;

    if (mqtt_topic_format(topic, sizeof(topic), s_device_id, "state") != PORT_OK) {
        return PORT_ERR_INVALID_ARG;
    }

    active = boot_bank_query_active();
    written = snprintf(payload, sizeof(payload),
                       "{\"online\": true, \"bank\": \"%c\"}",
                       (active == BOOT_BANK_B) ? 'B' : 'A');
    if (written <= 0 || (size_t)written >= sizeof(payload)) {
        return PORT_ERR_IO;
    }

    return mqtt->publish(topic, payload, (size_t)written, 1, true);
}

static port_err_t mqtt_client_subscribe_commands(const mqtt_port_t *mqtt)
{
    char topic[96];
    port_err_t err;

    if (mqtt_topic_format(topic, sizeof(topic), s_device_id, MQTT_CMD_WILDCARD) != PORT_OK) {
        return PORT_ERR_INVALID_ARG;
    }

    return mqtt->subscribe(topic, 1);
}

static void mqtt_client_on_message(const char *topic,
                                   const void *payload,
                                   size_t len,
                                   void *ctx)
{
    char *topic_copy;
    void *payload_copy;
    app_event_t ev;

    (void)ctx;

    if (topic == NULL || payload == NULL || len == 0u) {
        return;
    }

    topic_copy = pvPortMalloc(strlen(topic) + 1u);
    payload_copy = pvPortMalloc(len);
    if (topic_copy == NULL || payload_copy == NULL) {
        vPortFree(topic_copy);
        vPortFree(payload_copy);
        return;
    }

    memcpy(topic_copy, topic, strlen(topic) + 1u);
    memcpy(payload_copy, payload, len);

    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_MQTT_MESSAGE;
    ev.u.mqtt_message.topic = topic_copy;
    ev.u.mqtt_message.payload = payload_copy;
    ev.u.mqtt_message.len = len;
    (void)app_event_post(&ev);
}

static void mqtt_client_on_connection(bool connected, void *ctx)
{
    (void)ctx;
    (void)connected;
}

static port_err_t mqtt_client_do_connect(void)
{
    const mqtt_port_t *mqtt = mqtt_port_get();
    const config_port_t *cfg = config_port_get();
    mqtt_cred_t cred;

    if (s_suspended) {
        return PORT_ERR_BUSY;
    }
    mqtt_connect_cfg_t connect_cfg;
    mqtt_lwt_t lwt;
    char client_id[64];
    char state_topic[96];
    char mac_hex[13];
    port_err_t err;

    err = mqtt_cred_load(cfg, &cred);
    if (err != PORT_OK) {
        APP_LOG_E("mqtt", "no valid mqtt config in NVDM");
        return err;
    }

    mqtt_client_get_mac_hex(mac_hex, sizeof(mac_hex));
    mqtt_cred_resolve_device_id(&cred, mac_hex, s_device_id, sizeof(s_device_id));
    if (s_device_id[0] == '\0') {
        APP_LOG_E("mqtt", "device_id unavailable");
        return PORT_ERR_INVALID_ARG;
    }

    if (mqtt_client_id_format(client_id, sizeof(client_id), s_device_id) != PORT_OK) {
        return PORT_ERR_INVALID_ARG;
    }

    if (mqtt_topic_format(state_topic, sizeof(state_topic), s_device_id, "state") != PORT_OK) {
        return PORT_ERR_INVALID_ARG;
    }

    lwt.topic = state_topic;
    lwt.payload = MQTT_OFFLINE_PAYLOAD;
    lwt.qos = 1;
    lwt.retain = true;

    mqtt->set_lwt(&lwt);

    connect_cfg.host = cred.host;
    connect_cfg.port = cred.port;
    connect_cfg.client_id = client_id;
    connect_cfg.username = cred.user;
    connect_cfg.password = cred.pass;
    connect_cfg.lwt = lwt;

    if (s_log_connect_failures) {
        APP_LOG_I("mqtt", "connecting to %s:%u", cred.host, (unsigned)cred.port);
    }

    err = mqtt->connect(&connect_cfg);
    if (err != PORT_OK) {
        if (s_log_connect_failures) {
            APP_LOG_E("mqtt", "connect failed");
            s_log_connect_failures = false;
        }
        return err;
    }

    s_log_connect_failures = true;

    APP_LOG_I("mqtt", "connected");

    if (mqtt_client_subscribe_commands(mqtt) != PORT_OK) {
        APP_LOG_E("mqtt", "subscribe failed");
        mqtt->disconnect();
        return PORT_ERR_IO;
    }

    if (mqtt_client_publish_online(mqtt) != PORT_OK) {
        APP_LOG_E("mqtt", "online publish failed");
        mqtt->disconnect();
        return PORT_ERR_IO;
    }

    mqtt_backoff_on_success(&s_backoff);
    ota_client_set_device_id(s_device_id);

    {
        app_event_t ev;

        memset(&ev, 0, sizeof(ev));
        ev.type = EVT_MQTT_CONNECTED;
        (void)app_event_post(&ev);
    }

    return PORT_OK;
}

static void mqtt_client_connect_worker_reset(void)
{
    s_connect_worker_running = false;
    s_connect_worker_done = false;
    s_connect_worker = NULL;
}

static void mqtt_client_connect_worker_fn(void *param)
{
    (void)param;

    s_connect_worker_result = mqtt_client_do_connect();
    s_connect_worker_running = false;
    s_connect_worker_done = true;
    s_connect_worker = NULL;
    vTaskDelete(NULL);
}

static void mqtt_client_begin_connect_job(void)
{
    if (s_suspended) {
        return;
    }

    if (s_connect_worker_running || s_connect_worker_done) {
        return;
    }

    mqtt_client_sync_session(mqtt_port_get());
    s_connect_worker_running = true;
    s_connect_worker_done = false;

    if (s_unit_test_mode) {
        s_connect_worker_result = mqtt_client_do_connect();
        s_connect_worker_running = false;
        s_connect_worker_done = true;
        return;
    }

    if (xTaskCreate(mqtt_client_connect_worker_fn,
                    "mqtt_cn",
                    MQTT_CONNECT_WORKER_STACK / sizeof(portSTACK_TYPE),
                    NULL,
                    MQTT_CONNECT_WORKER_PRIO,
                    &s_connect_worker) != pdPASS) {
        APP_LOG_E("mqtt", "connect worker create failed; inline fallback");
        s_connect_worker_result = mqtt_client_do_connect();
        s_connect_worker_running = false;
        s_connect_worker_done = true;
    }
}

static void mqtt_client_do_disconnect(const mqtt_port_t *mqtt)
{
    if (mqtt != NULL) {
        mqtt->disconnect();
    }

    s_connect_pending = false;
    s_connect_busy = false;
    s_reconnect_at = 0;
    mqtt_client_connect_worker_reset();
}

uint32_t mqtt_client_step(void)
{
    const mqtt_port_t *mqtt = mqtt_port_get();
    uint32_t delay_ms = 500;

    if (s_disconnect_pending) {
        s_disconnect_pending = false;
        APP_LOG_I("mqtt", "disconnecting");
        mqtt_client_do_disconnect(mqtt);
        delay_ms = 200;
        goto done;
    }

    if (s_suspended) {
        delay_ms = 200;
        goto done;
    }

    if (!s_connect_armed) {
        delay_ms = 500;
        goto done;
    }

    if (!s_wifi_ready || !mqtt_client_wifi_has_ip()) {
        delay_ms = 500;
        goto done;
    }

    if (!s_connect_pending && !s_connect_worker_running && mqtt->is_connected()) {
        mqtt_adapter_yield(250);
        delay_ms = 250;
        goto done;
    }

    if (!s_connect_pending) {
        if (s_reconnect_at != 0 && xTaskGetTickCount() < s_reconnect_at) {
            delay_ms = 200;
            goto done;
        }
        if (!mqtt_cred_is_stored(config_port_get())) {
            delay_ms = 500;
            goto done;
        }
        s_connect_pending = true;
    }

    if (!s_connect_worker_running && mqtt->is_connected()) {
        s_connect_pending = false;
        s_connect_busy = false;
        mqtt_adapter_yield(250);
        delay_ms = 250;
        goto done;
    }

    if (!s_connect_worker_running && !s_connect_worker_done) {
        mqtt_client_begin_connect_job();
    }

    if (s_connect_worker_running) {
        delay_ms = MQTT_CONNECT_POLL_MS;
        goto done;
    }

    if (s_connect_worker_done) {
        port_err_t connect_err = s_connect_worker_result;

        s_connect_worker_done = false;

        if (!s_connect_armed) {
            mqtt->disconnect();
            delay_ms = 200;
            goto done;
        }

        if (connect_err == PORT_OK) {
            s_connect_pending = false;
            s_connect_busy = false;
            s_reconnect_at = 0;
            delay_ms = 0;
            goto done;
        }

        mqtt->disconnect();
        mqtt_backoff_on_failure(&s_backoff);
        s_reconnect_at =
            xTaskGetTickCount() + pdMS_TO_TICKS(mqtt_backoff_current_ms(&s_backoff));
        s_connect_pending = false;
        s_connect_busy = false;
        delay_ms = 200;
        goto done;
    }

    delay_ms = MQTT_CONNECT_POLL_MS;

done:
    mqtt_client_sync_session(mqtt);
    return delay_ms;
}

static void mqtt_client_task(void *param)
{
    (void)param;

    for (;;) {
        uint32_t delay_ms = mqtt_client_step();

        if (delay_ms > 0) {
            vTaskDelay(pdMS_TO_TICKS(delay_ms));
        }
    }
}

/* Host-test entry points — not called from firmware; stripped by --gc-sections if unused. */
void mqtt_client_test_reset(void)
{
    app_event_port_init();
    s_mqtt_task = NULL;
    s_unit_test_mode = false;
    mqtt_client_connect_worker_reset();
    s_connect_busy = false;
    s_wifi_ready = false;
    s_connect_pending = false;
    s_connect_armed = false;
    s_disconnect_pending = false;
    s_suspended = false;
    s_log_connect_failures = true;
    s_reconnect_at = 0;
    s_session_phase = MQTT_SESSION_OFF;
    s_device_id[0] = '\0';
    mqtt_backoff_init(&s_backoff);
}

void mqtt_client_test_bootstrap(void)
{
    mqtt_client_test_reset();
    s_mqtt_task = (TaskHandle_t)1;
    s_unit_test_mode = true;
    mqtt_port_get()->set_callbacks(mqtt_client_on_message, mqtt_client_on_connection, NULL);
}

void mqtt_client_test_start(void)
{
    mqtt_client_test_bootstrap();
}

void mqtt_client_test_set_device_id(const char *device_id)
{
    if (device_id == NULL) {
        s_device_id[0] = '\0';
        return;
    }

    strncpy(s_device_id, device_id, sizeof(s_device_id) - 1);
    s_device_id[sizeof(s_device_id) - 1] = '\0';
}

bool mqtt_client_test_is_suspended(void)
{
    return s_suspended;
}

void mqtt_client_start(void)
{
    mqtt_backoff_init(&s_backoff);
    s_reconnect_at = 0;
    s_log_connect_failures = true;
    mqtt_port_get()->set_callbacks(mqtt_client_on_message, mqtt_client_on_connection, NULL);
    if (xTaskCreate(mqtt_client_task,
                    "mqtt_io",
                    4096 / sizeof(portSTACK_TYPE),
                    NULL,
                    TASK_PRIORITY_ABOVE_NORMAL,
                    &s_mqtt_task) != pdPASS) {
        APP_LOG_E("mqtt", "failed to start mqtt_io task");
        return;
    }
}

bool mqtt_client_wifi_is_ready(void)
{
    return s_wifi_ready && mqtt_client_wifi_has_ip();
}

bool mqtt_client_connect_in_progress(void)
{
    return s_connect_busy || s_connect_worker_running;
}

bool mqtt_client_request_connect(void)
{
    if (s_mqtt_task == NULL) {
        return false;
    }

    if (s_suspended) {
        return false;
    }

    if (!mqtt_client_wifi_is_ready()) {
        return false;
    }

    if (s_connect_busy) {
        return false;
    }

    s_connect_armed = true;
    s_log_connect_failures = true;
    s_connect_busy = true;
    s_connect_pending = true;
    mqtt_client_sync_session(mqtt_port_get());
    return true;
}

void mqtt_client_stop(void)
{
    s_connect_armed = false;
    s_connect_pending = false;
    s_connect_busy = false;
    mqtt_client_connect_worker_reset();
    s_reconnect_at = 0;
    s_log_connect_failures = true;
    mqtt_backoff_init(&s_backoff);
    s_disconnect_pending = true;
}

void mqtt_client_notify_wifi_ready(void)
{
    s_wifi_ready = true;
}

const char *mqtt_client_device_id(void)
{
    return s_device_id;
}

void mqtt_client_suspend_for_ota(void)
{
    s_suspended = true;
    s_connect_armed = false;
    s_connect_pending = false;
    s_connect_busy = false;
    s_disconnect_pending = true;
    s_reconnect_at = 0;
    mqtt_client_connect_worker_reset();
    APP_LOG_I("mqtt", "suspended for ota");
}

void mqtt_client_resume_after_ota(void)
{
    s_suspended = false;

    if (mqtt_cred_is_stored(config_port_get()) && mqtt_client_wifi_is_ready()) {
        s_connect_armed = true;
        mqtt_client_request_connect();
    }

    APP_LOG_I("mqtt", "resumed after ota");
}

bool mqtt_client_wait_disconnected(uint32_t timeout_ms)
{
    const mqtt_port_t *mqtt = mqtt_port_get();
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

    while (xTaskGetTickCount() < deadline) {
        if (s_unit_test_mode) {
            mqtt_client_step();
        }

        if (mqtt == NULL || !mqtt->is_connected()) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    return (mqtt == NULL || !mqtt->is_connected());
}
