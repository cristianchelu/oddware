/*
 * MQTT port adapter — spec/40-architecture/ports.md
 */

#include <stdio.h>
#include <string.h>

#include "MQTTClient.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "app_log.h"
#include "mqtt_cred.h"
#include "mqtt_topics.h"
#include "provision_form.h"

#include "mqtt_adapter.h"
#include "mqtt_port.h"

#define MQTT_CMD_TIMEOUT_MS     12000
#define MQTT_MUTEX_WAIT_MS      12000
#define MQTT_TX_BUF_SIZE        1024
#define MQTT_RX_BUF_SIZE        512

static Network s_network;
static SemaphoreHandle_t s_mqtt_mutex;
static Client s_client;
static unsigned char s_tx_buf[MQTT_TX_BUF_SIZE];
static unsigned char s_rx_buf[MQTT_RX_BUF_SIZE];
static char s_port_str[8];
static bool s_session_up;
static mqtt_lwt_t s_lwt;
static char s_lwt_topic[128];
static char s_lwt_payload[128];
static char s_subscribe_topic[128];
static char s_connect_client_id[64];
static mqtt_message_cb_t s_on_message;
static mqtt_connection_cb_t s_on_connection;
static void *s_cb_ctx;

static void mqtt_adapter_notify_connection(bool connected);

static void mqtt_mutex_ensure(void)
{
    if (s_mqtt_mutex == NULL) {
        s_mqtt_mutex = xSemaphoreCreateMutex();
    }
}

static bool mqtt_mutex_take(void)
{
    mqtt_mutex_ensure();
    if (s_mqtt_mutex == NULL) {
        return false;
    }

    return xSemaphoreTake(s_mqtt_mutex, pdMS_TO_TICKS(MQTT_MUTEX_WAIT_MS)) == pdPASS;
}

static void mqtt_mutex_give(void)
{
    if (s_mqtt_mutex != NULL) {
        (void)xSemaphoreGive(s_mqtt_mutex);
    }
}

static void mqtt_adapter_yield_locked(int timeout_ms)
{
    int rc;

    if (!s_session_up) {
        return;
    }

    rc = MQTTYield(&s_client, timeout_ms);
    (void)rc;
    if (!s_client.isconnected && s_session_up) {
        s_network.disconnect(&s_network);
        mqtt_adapter_notify_connection(false);
    }
}

static void mqtt_adapter_message_arrived(MessageData *md)
{
    MQTTMessage *message;
    MQTTString *topic;
    const char *topic_cstr;
    char topic_buf[128];

    if (md == NULL || s_on_message == NULL) {
        return;
    }

    message = md->message;
    topic = md->topicName;
    if (topic == NULL || message == NULL) {
        return;
    }

    if (topic->cstring != NULL) {
        topic_cstr = topic->cstring;
    } else if (topic->lenstring.len > 0 && topic->lenstring.data != NULL) {
        size_t copy_len = topic->lenstring.len;

        if (copy_len >= sizeof(topic_buf)) {
            copy_len = sizeof(topic_buf) - 1;
        }
        memcpy(topic_buf, topic->lenstring.data, copy_len);
        topic_buf[copy_len] = '\0';
        topic_cstr = topic_buf;
    } else {
        APP_LOG_E("mqtt", "message dropped: no topic");
        return;
    }

    s_on_message(topic_cstr, message->payload, message->payloadlen, s_cb_ctx);
}

static void mqtt_adapter_notify_connection(bool connected)
{
    s_session_up = connected;
    if (s_on_connection != NULL) {
        s_on_connection(connected, s_cb_ctx);
    }
}

static port_err_t mqtt_adapter_connect(const mqtt_connect_cfg_t *cfg)
{
    MQTTPacket_connectData data = MQTTPacket_connectData_initializer;
    int rc;

    if (cfg == NULL || cfg->host == NULL || cfg->client_id == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (s_session_up) {
        return PORT_OK;
    }

    if (!mqtt_mutex_take()) {
        return PORT_ERR_IO;
    }

    NewNetwork(&s_network);
    snprintf(s_port_str, sizeof(s_port_str), "%u", (unsigned)cfg->port);

    rc = ConnectNetwork(&s_network, (char *)cfg->host, s_port_str);
    if (rc != 0) {
        mqtt_mutex_give();
        return PORT_ERR_IO;
    }
    taskYIELD();

    MQTTClient(&s_client,
               &s_network,
               MQTT_CMD_TIMEOUT_MS,
               s_tx_buf,
               sizeof(s_tx_buf),
               s_rx_buf,
               sizeof(s_rx_buf));

    data.MQTTVersion = 4;
    strncpy(s_connect_client_id, cfg->client_id, sizeof(s_connect_client_id) - 1);
    s_connect_client_id[sizeof(s_connect_client_id) - 1] = '\0';
    data.clientID.cstring = s_connect_client_id;
    data.keepAliveInterval = 60;
    data.cleansession = 1;

    if (cfg->username != NULL && cfg->username[0] != '\0') {
        data.username.cstring = (char *)cfg->username;
        data.password.cstring = (char *)(cfg->password != NULL ? cfg->password : "");
    }

    if (s_lwt.topic != NULL && s_lwt.payload != NULL) {
        data.willFlag = 1;
        data.will.topicName.cstring = s_lwt_topic;
        data.will.message.cstring = s_lwt_payload;
        data.will.qos = s_lwt.qos;
        data.will.retained = s_lwt.retain ? 1 : 0;
    } else if (cfg->lwt.topic != NULL && cfg->lwt.payload != NULL) {
        data.willFlag = 1;
        data.will.topicName.cstring = (char *)cfg->lwt.topic;
        data.will.message.cstring = (char *)cfg->lwt.payload;
        data.will.qos = cfg->lwt.qos;
        data.will.retained = cfg->lwt.retain ? 1 : 0;
    }

    rc = MQTTConnect(&s_client, &data);
    if (rc != 0) {
        s_network.disconnect(&s_network);
        mqtt_mutex_give();
        return PORT_ERR_IO;
    }
    taskYIELD();

    mqtt_adapter_notify_connection(true);
    mqtt_mutex_give();
    return PORT_OK;
}

static port_err_t mqtt_adapter_disconnect(void)
{
    if (!s_session_up) {
        return PORT_OK;
    }

    if (!mqtt_mutex_take()) {
        return PORT_ERR_IO;
    }

    MQTTDisconnect(&s_client);
    s_network.disconnect(&s_network);
    mqtt_adapter_notify_connection(false);
    mqtt_mutex_give();
    return PORT_OK;
}

static port_err_t mqtt_adapter_publish(const char *topic,
                                       const void *payload,
                                       size_t len,
                                       uint8_t qos,
                                       bool retain)
{
    MQTTMessage message;
    int rc;

    if (!s_session_up || topic == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (len > 0u && payload == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (!mqtt_mutex_take()) {
        return PORT_ERR_IO;
    }

    mqtt_adapter_yield_locked(10);

    memset(&message, 0, sizeof(message));
    message.qos = qos;
    message.retained = retain ? 1 : 0;
    message.payload = len > 0u ? (void *)payload : NULL;
    message.payloadlen = len;

    rc = MQTTPublish(&s_client, topic, &message);
    if (rc != 0) {
        APP_LOG_E("mqtt", "publish failed topic=%s", topic);
    } else {
        mqtt_adapter_yield_locked(50);
    }

    mqtt_mutex_give();
    return rc == 0 ? PORT_OK : PORT_ERR_IO;
}

static port_err_t mqtt_adapter_subscribe(const char *topic, uint8_t qos)
{
    int rc;

    if (!s_session_up || topic == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (!mqtt_mutex_take()) {
        return PORT_ERR_IO;
    }

    strncpy(s_subscribe_topic, topic, sizeof(s_subscribe_topic) - 1);
    s_subscribe_topic[sizeof(s_subscribe_topic) - 1] = '\0';

    rc = MQTTSubscribe(&s_client, s_subscribe_topic, qos, mqtt_adapter_message_arrived);
    if (rc != 0) {
        APP_LOG_E("mqtt", "subscribe failed topic=%s", topic);
    } else {
        mqtt_adapter_yield_locked(50);
    }

    mqtt_mutex_give();
    return rc == 0 ? PORT_OK : PORT_ERR_IO;
}

static port_err_t mqtt_adapter_set_lwt(const mqtt_lwt_t *lwt)
{
    if (lwt == NULL || lwt->topic == NULL || lwt->payload == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    strncpy(s_lwt_topic, lwt->topic, sizeof(s_lwt_topic) - 1);
    strncpy(s_lwt_payload, lwt->payload, sizeof(s_lwt_payload) - 1);
    s_lwt.topic = s_lwt_topic;
    s_lwt.payload = s_lwt_payload;
    s_lwt.qos = lwt->qos;
    s_lwt.retain = lwt->retain;
    return PORT_OK;
}

static bool mqtt_adapter_is_connected(void)
{
    return s_session_up && s_client.isconnected;
}

static void mqtt_adapter_set_callbacks(mqtt_message_cb_t on_message,
                                       mqtt_connection_cb_t on_connection,
                                       void *ctx)
{
    s_on_message = on_message;
    s_on_connection = on_connection;
    s_cb_ctx = ctx;
}

static const mqtt_port_t s_mqtt_port = {
    .connect = mqtt_adapter_connect,
    .disconnect = mqtt_adapter_disconnect,
    .publish = mqtt_adapter_publish,
    .subscribe = mqtt_adapter_subscribe,
    .set_lwt = mqtt_adapter_set_lwt,
    .is_connected = mqtt_adapter_is_connected,
    .set_callbacks = mqtt_adapter_set_callbacks,
};

const mqtt_port_t *mqtt_port_get(void)
{
    return &s_mqtt_port;
}

bool mqtt_adapter_probe_broker(const provision_input_t *input,
                               const char *mac_hex12,
                               uint32_t timeout_ms)
{
    const mqtt_port_t *mqtt = mqtt_port_get();
    mqtt_connect_cfg_t cfg;
    mqtt_cred_t cred;
    char device_id[MQTT_DEVICE_ID_MAX_LEN + 1];
    char client_id[MQTT_DEVICE_ID_MAX_LEN + 16];
    TickType_t deadline;
    uint16_t port;

    if (input == NULL || mac_hex12 == NULL) {
        return false;
    }

    memset(&cred, 0, sizeof(cred));
    strncpy(cred.host, input->mqtt_host, sizeof(cred.host) - 1);
    port = input->mqtt_port_set ? input->mqtt_port : 1883;
    cred.port = port;
    strncpy(cred.user, input->mqtt_user, sizeof(cred.user) - 1);
    strncpy(cred.pass, input->mqtt_pass, sizeof(cred.pass) - 1);
    strncpy(cred.device_id, input->device_id, sizeof(cred.device_id) - 1);

    mqtt_cred_resolve_device_id(&cred, mac_hex12, device_id, sizeof(device_id));
    if (mqtt_client_id_format(client_id, sizeof(client_id), device_id) != PORT_OK) {
        return false;
    }

    memset(&cfg, 0, sizeof(cfg));
    cfg.host = cred.host;
    cfg.port = port;
    cfg.client_id = client_id;
    cfg.username = cred.user;
    cfg.password = cred.pass;

    if (mqtt->connect(&cfg) != PORT_OK) {
        mqtt->disconnect();
        return false;
    }

    deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);
    while (xTaskGetTickCount() < deadline) {
        mqtt_adapter_yield(200);
        if (mqtt->is_connected()) {
            mqtt->disconnect();
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(100));
    }

    mqtt->disconnect();
    return false;
}

void mqtt_adapter_yield(int timeout_ms)
{
    if (!s_session_up) {
        return;
    }

    if (!mqtt_mutex_take()) {
        return;
    }

    mqtt_adapter_yield_locked(timeout_ms);
    mqtt_mutex_give();
}
