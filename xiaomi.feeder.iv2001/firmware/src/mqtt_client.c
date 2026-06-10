/*
 * MQTT client orchestration — spec/30-processes/mqtt-protocol.md
 */

#include <stdio.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "syslog.h"
#include "wifi_api.h"

#include "task_def.h"

#include "boot_bank_target.h"
#include "config_port.h"
#include "display_mqtt_indicator.h"
#include "mqtt_adapter.h"
#include "mqtt_backoff.h"
#include "mqtt_client.h"
#include "mqtt_cred.h"
#include "mqtt_port.h"
#include "mqtt_route.h"
#include "mqtt_topics.h"
#include "ota_client.h"
#include "wifi_port.h"

log_create_module(mqtt_client, PRINT_LEVEL_INFO);

#define MQTT_OFFLINE_PAYLOAD  "{\"online\": false}"
#define MQTT_CMD_WILDCARD     "cmd/#"

static TaskHandle_t s_mqtt_task;
static volatile bool s_connect_busy;
static volatile bool s_wifi_ready;
static volatile bool s_connect_pending;
static volatile bool s_connect_armed;
static volatile bool s_disconnect_pending;
static volatile bool s_suspended;
static bool s_boot_autoconnect;
static bool s_log_connect_failures;
static mqtt_backoff_t s_backoff;
static TickType_t s_reconnect_at;
static char s_device_id[MQTT_DEVICE_ID_MAX_LEN + 1];

typedef enum {
    MQTT_DISPLAY_OFF,
    MQTT_DISPLAY_CONNECTING,
    MQTT_DISPLAY_CONNECTED,
    MQTT_DISPLAY_ERROR,
} mqtt_display_state_t;

static mqtt_display_state_t s_display_state = MQTT_DISPLAY_OFF;

static void mqtt_client_set_display(mqtt_display_state_t state)
{
    if (state == s_display_state) {
        return;
    }

    s_display_state = state;

    switch (state) {
    case MQTT_DISPLAY_CONNECTING:
        display_mqtt_indicator_connecting();
        break;
    case MQTT_DISPLAY_CONNECTED:
        display_mqtt_indicator_connected();
        break;
    case MQTT_DISPLAY_ERROR:
        display_mqtt_indicator_error();
        break;
    default:
        display_mqtt_indicator_off();
        break;
    }
}

static mqtt_display_state_t mqtt_client_derive_display_state(const mqtt_port_t *mqtt)
{
    if (s_suspended || !s_connect_armed || !mqtt_client_wifi_is_ready()) {
        return MQTT_DISPLAY_OFF;
    }

    if (mqtt != NULL && mqtt->is_connected()) {
        return MQTT_DISPLAY_CONNECTED;
    }

    if (s_reconnect_at != 0 && xTaskGetTickCount() < s_reconnect_at) {
        return MQTT_DISPLAY_ERROR;
    }

    if (s_connect_busy || s_connect_pending) {
        return MQTT_DISPLAY_CONNECTING;
    }

    return MQTT_DISPLAY_OFF;
}

static void mqtt_client_sync_display(const mqtt_port_t *mqtt)
{
    mqtt_client_set_display(mqtt_client_derive_display_state(mqtt));
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

    err = mqtt->subscribe(topic, 1);
    if (err == PORT_OK) {
        LOG_I(mqtt_client, "subscribed %s", topic);
        printf("[mqtt] subscribed %s\r\n", topic);
    }
    return err;
}

static void mqtt_client_on_message(const char *topic,
                                   const void *payload,
                                   size_t len,
                                   void *ctx)
{
    (void)ctx;

    ota_client_on_mqtt_message(topic, payload, len);
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
    mqtt_connect_cfg_t connect_cfg;
    mqtt_lwt_t lwt;
    char client_id[64];
    char state_topic[96];
    char mac_hex[13];
    port_err_t err;

    err = mqtt_cred_load(cfg, &cred);
    if (err != PORT_OK) {
        LOG_E(mqtt_client, "no valid mqtt config in NVDM");
        return err;
    }

    mqtt_client_get_mac_hex(mac_hex, sizeof(mac_hex));
    mqtt_cred_resolve_device_id(&cred, mac_hex, s_device_id, sizeof(s_device_id));
    if (s_device_id[0] == '\0') {
        LOG_E(mqtt_client, "device_id unavailable");
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
        LOG_I(mqtt_client, "connecting to %s:%u", cred.host, (unsigned)cred.port);
        printf("[mqtt] connecting to %s:%u\r\n", cred.host, (unsigned)cred.port);
    }

    err = mqtt->connect(&connect_cfg);
    if (err != PORT_OK) {
        if (s_log_connect_failures) {
            LOG_E(mqtt_client, "mqtt connect failed");
            printf("[mqtt] connect failed\r\n");
            s_log_connect_failures = false;
        }
        return err;
    }

    s_log_connect_failures = true;

    LOG_I(mqtt_client, "mqtt connected");
    printf("[mqtt] connected\r\n");

    if (mqtt_client_subscribe_commands(mqtt) != PORT_OK) {
        LOG_E(mqtt_client, "subscribe failed");
        mqtt->disconnect();
        return PORT_ERR_IO;
    }

    if (mqtt_client_publish_online(mqtt) != PORT_OK) {
        LOG_E(mqtt_client, "online publish failed");
        mqtt->disconnect();
        return PORT_ERR_IO;
    }

    mqtt_backoff_on_success(&s_backoff);
    ota_client_set_device_id(s_device_id);
    ota_client_on_mqtt_connected();
    return PORT_OK;
}

static void mqtt_client_do_disconnect(const mqtt_port_t *mqtt)
{
    if (mqtt != NULL) {
        mqtt->disconnect();
    }

    s_connect_pending = false;
    s_connect_busy = false;
    s_reconnect_at = 0;
}

static uint32_t mqtt_client_cap_delay(uint32_t delay_ms, uint32_t ota_delay)
{
    if (ota_delay > 0 && ota_delay < delay_ms) {
        return ota_delay;
    }

    return delay_ms;
}

uint32_t mqtt_client_step(void)
{
    const mqtt_port_t *mqtt = mqtt_port_get();
    uint32_t ota_delay;
    uint32_t delay_ms = 500;

    if (s_disconnect_pending) {
        s_disconnect_pending = false;
        LOG_I(mqtt_client, "disconnecting");
        mqtt_client_do_disconnect(mqtt);
        delay_ms = 200;
        goto done;
    }

    if (s_suspended) {
        ota_delay = ota_client_poll_ms();
        delay_ms = mqtt_client_cap_delay(200, ota_delay);
        goto done;
    }

    ota_delay = ota_client_poll_ms();

    if (!s_connect_armed) {
        delay_ms = mqtt_client_cap_delay(500, ota_delay);
        goto done;
    }

    if (!s_wifi_ready || !mqtt_client_wifi_has_ip()) {
        delay_ms = mqtt_client_cap_delay(500, ota_delay);
        goto done;
    }

    if (!s_connect_pending && mqtt->is_connected()) {
        mqtt_adapter_yield(250);
        ota_delay = ota_client_poll_ms();
        delay_ms = mqtt_client_cap_delay(250, ota_delay);
        goto done;
    }

    if (!s_connect_pending) {
        if (s_reconnect_at != 0 && xTaskGetTickCount() < s_reconnect_at) {
            delay_ms = mqtt_client_cap_delay(200, ota_delay);
            goto done;
        }
        if (!mqtt_cred_is_stored(config_port_get())) {
            delay_ms = mqtt_client_cap_delay(500, ota_delay);
            goto done;
        }
        s_connect_pending = true;
    }

    if (mqtt->is_connected()) {
        s_connect_pending = false;
        s_connect_busy = false;
        mqtt_adapter_yield(250);
        ota_delay = ota_client_poll_ms();
        delay_ms = mqtt_client_cap_delay(250, ota_delay);
        goto done;
    }

    if (mqtt_client_do_connect() == PORT_OK) {
        s_connect_pending = false;
        s_connect_busy = false;
        s_reconnect_at = 0;
        delay_ms = 0;
        goto done;
    }

    mqtt->disconnect();
    mqtt_backoff_on_failure(&s_backoff);
    s_reconnect_at = xTaskGetTickCount() + pdMS_TO_TICKS(mqtt_backoff_current_ms(&s_backoff));
    s_connect_pending = false;
    s_connect_busy = false;
    delay_ms = mqtt_client_cap_delay(200, ota_delay);

done:
    mqtt_client_sync_display(mqtt);
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
    s_mqtt_task = NULL;
    s_connect_busy = false;
    s_wifi_ready = false;
    s_connect_pending = false;
    s_connect_armed = false;
    s_disconnect_pending = false;
    s_suspended = false;
    s_boot_autoconnect = false;
    s_log_connect_failures = true;
    s_reconnect_at = 0;
    s_display_state = MQTT_DISPLAY_OFF;
    s_device_id[0] = '\0';
    mqtt_backoff_init(&s_backoff);
}

void mqtt_client_test_bootstrap(void)
{
    mqtt_client_test_reset();
    s_mqtt_task = (TaskHandle_t)1;
    mqtt_port_get()->set_callbacks(mqtt_client_on_message, mqtt_client_on_connection, NULL);
}

void mqtt_client_test_start(void)
{
    mqtt_client_test_bootstrap();
    s_boot_autoconnect = mqtt_cred_is_stored(config_port_get());
}

void mqtt_client_start(void)
{
    mqtt_backoff_init(&s_backoff);
    s_reconnect_at = 0;
    s_log_connect_failures = true;
    mqtt_port_get()->set_callbacks(mqtt_client_on_message, mqtt_client_on_connection, NULL);
    s_boot_autoconnect = mqtt_cred_is_stored(config_port_get());

    if (xTaskCreate(mqtt_client_task,
                    "mqtt_io",
                    4096 / sizeof(portSTACK_TYPE),
                    NULL,
                    TASK_PRIORITY_ABOVE_NORMAL,
                    &s_mqtt_task) != pdPASS) {
        LOG_E(mqtt_client, "failed to start mqtt_io task");
        return;
    }
}

bool mqtt_client_wifi_is_ready(void)
{
    return s_wifi_ready && mqtt_client_wifi_has_ip();
}

bool mqtt_client_connect_in_progress(void)
{
    return s_connect_busy;
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
    mqtt_client_set_display(MQTT_DISPLAY_CONNECTING);
    return true;
}

void mqtt_client_stop(void)
{
    s_connect_armed = false;
    s_boot_autoconnect = false;
    s_connect_pending = false;
    s_connect_busy = false;
    s_reconnect_at = 0;
    s_log_connect_failures = true;
    mqtt_backoff_init(&s_backoff);
    s_disconnect_pending = true;
    mqtt_client_set_display(MQTT_DISPLAY_OFF);
}

void mqtt_client_notify_wifi_ready(void)
{
    s_wifi_ready = true;

    if (s_suspended) {
        return;
    }

    if (s_boot_autoconnect && mqtt_cred_is_stored(config_port_get())) {
        s_boot_autoconnect = false;
        mqtt_client_request_connect();
        return;
    }

    if (s_connect_armed) {
        mqtt_client_request_connect();
    }
}

void mqtt_client_suspend_for_ota(void)
{
    s_suspended = true;
    s_disconnect_pending = true;
    printf("[mqtt] suspended for ota\r\n");
}

void mqtt_client_resume_after_ota(void)
{
    s_suspended = false;

    if (s_mqtt_task != NULL) {
        vTaskResume(s_mqtt_task);
        printf("[mqtt] io task resumed\r\n");
    }

    if (mqtt_cred_is_stored(config_port_get()) && mqtt_client_wifi_is_ready()) {
        s_connect_armed = true;
        mqtt_client_request_connect();
    }

    printf("[mqtt] resumed after ota\r\n");
}

bool mqtt_client_wait_disconnected(uint32_t timeout_ms)
{
    const mqtt_port_t *mqtt = mqtt_port_get();
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

    while (xTaskGetTickCount() < deadline) {
        if (mqtt == NULL || !mqtt->is_connected()) {
            if (s_mqtt_task != NULL) {
                vTaskSuspend(s_mqtt_task);
                printf("[mqtt] io task suspended\r\n");
            }
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    return (mqtt == NULL || !mqtt->is_connected());
}
