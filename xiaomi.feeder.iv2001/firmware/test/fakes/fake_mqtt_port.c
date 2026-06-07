#include <string.h>

#include "fake_mqtt_port.h"

static fake_mqtt_port_state_t s_state;
static mqtt_message_cb_t s_on_message;
static mqtt_connection_cb_t s_on_connection;
static void *s_cb_ctx;

void fake_mqtt_port_reset(void)
{
    memset(&s_state, 0, sizeof(s_state));
    s_on_message = NULL;
    s_on_connection = NULL;
    s_cb_ctx = NULL;
}

void fake_mqtt_port_set_fail_next_connect(bool fail)
{
    s_state.fail_next_connect = fail;
}

const fake_mqtt_port_state_t *fake_mqtt_port_state(void)
{
    return &s_state;
}

static port_err_t fake_mqtt_connect(const mqtt_connect_cfg_t *cfg)
{
    (void)cfg;

    s_state.connect_calls++;
    if (s_state.fail_next_connect) {
        s_state.fail_next_connect = false;
        return PORT_ERR_IO;
    }

    s_state.connected = true;
    if (s_on_connection != NULL) {
        s_on_connection(true, s_cb_ctx);
    }

    return PORT_OK;
}

static port_err_t fake_mqtt_disconnect(void)
{
    s_state.disconnect_calls++;
    if (s_state.connected && s_on_connection != NULL) {
        s_on_connection(false, s_cb_ctx);
    }
    s_state.connected = false;
    return PORT_OK;
}

static port_err_t fake_mqtt_publish(const char *topic,
                                    const void *payload,
                                    size_t len,
                                    uint8_t qos,
                                    bool retain)
{
    (void)qos;
    (void)retain;

    if (!s_state.connected) {
        return PORT_ERR_INVALID_ARG;
    }

    if (s_state.fail_publish) {
        return PORT_ERR_IO;
    }

    s_state.publish_calls++;
    if (topic != NULL) {
        strncpy(s_state.last_publish_topic, topic, sizeof(s_state.last_publish_topic) - 1);
    }
    if (payload != NULL && len > 0) {
        size_t copy_len = len;

        if (copy_len >= sizeof(s_state.last_publish_payload)) {
            copy_len = sizeof(s_state.last_publish_payload) - 1;
        }
        memcpy(s_state.last_publish_payload, payload, copy_len);
        s_state.last_publish_payload[copy_len] = '\0';
    }

    return PORT_OK;
}

static port_err_t fake_mqtt_subscribe(const char *topic, uint8_t qos)
{
    (void)qos;

    if (!s_state.connected) {
        return PORT_ERR_INVALID_ARG;
    }

    if (s_state.fail_subscribe) {
        return PORT_ERR_IO;
    }

    s_state.subscribe_calls++;
    if (topic != NULL) {
        strncpy(s_state.last_subscribe_topic, topic, sizeof(s_state.last_subscribe_topic) - 1);
    }

    return PORT_OK;
}

static port_err_t fake_mqtt_set_lwt(const mqtt_lwt_t *lwt)
{
    (void)lwt;
    return PORT_OK;
}

static bool fake_mqtt_is_connected(void)
{
    return s_state.connected;
}

static void fake_mqtt_set_callbacks(mqtt_message_cb_t on_message,
                                    mqtt_connection_cb_t on_connection,
                                    void *ctx)
{
    s_on_message = on_message;
    s_on_connection = on_connection;
    s_cb_ctx = ctx;
}

static const mqtt_port_t s_fake_mqtt_port = {
    .connect = fake_mqtt_connect,
    .disconnect = fake_mqtt_disconnect,
    .publish = fake_mqtt_publish,
    .subscribe = fake_mqtt_subscribe,
    .set_lwt = fake_mqtt_set_lwt,
    .is_connected = fake_mqtt_is_connected,
    .set_callbacks = fake_mqtt_set_callbacks,
};

const mqtt_port_t *fake_mqtt_port_get(void)
{
    return &s_fake_mqtt_port;
}
