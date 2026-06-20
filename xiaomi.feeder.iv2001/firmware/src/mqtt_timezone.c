/*
 * Device timezone MQTT publisher — spec/30-processes/mqtt-protocol.md § Device timezone
 */

#include <string.h>

#include "app_log.h"
#include "mqtt_outbox.h"
#include "mqtt_timezone.h"
#include "mqtt_topics.h"
#include "port_err.h"
#include "time_config.h"
#include "tz_rule.h"

static char s_timezone_topic[96];
static char s_last_payload[TZ_RULE_POSIX_MAX];
static bool s_last_payload_valid;

static bool mqtt_timezone_publish_payload(const char *payload)
{
    size_t payload_len;

    if (payload == NULL || s_timezone_topic[0] == '\0') {
        return false;
    }

    payload_len = strlen(payload);
    if (s_last_payload_valid && strcmp(s_last_payload, payload) == 0) {
        return true;
    }

    if (!mqtt_outbox_enqueue(s_timezone_topic,
                             payload,
                             payload_len,
                             1,
                             true)) {
        app_log_debug("mqtt", "timezone enqueue failed topic=%s", s_timezone_topic);
        return false;
    }

    if (payload_len + 1 <= sizeof(s_last_payload)) {
        memcpy(s_last_payload, payload, payload_len + 1);
        s_last_payload_valid = true;
    }

    app_log_info("app", "timezone %s", payload);
    return true;
}

void mqtt_timezone_set_device_id(const char *device_id)
{
    if (device_id == NULL || device_id[0] == '\0') {
        s_timezone_topic[0] = '\0';
        return;
    }

    if (mqtt_topic_format(s_timezone_topic,
                          sizeof(s_timezone_topic),
                          device_id,
                          "timezone")
            != PORT_OK) {
        s_timezone_topic[0] = '\0';
    }
}

void mqtt_timezone_publish_snapshot(void)
{
    char payload[TZ_RULE_POSIX_MAX];

    if (!time_config_format_timezone_display(payload, sizeof(payload))) {
        return;
    }

    (void)mqtt_timezone_publish_payload(payload);
}

void mqtt_timezone_connect_snapshot(void)
{
    if (s_timezone_topic[0] == '\0') {
        return;
    }

    s_last_payload_valid = false;
    mqtt_timezone_publish_snapshot();
}

void mqtt_timezone_on_outbox_reset(void)
{
    s_last_payload_valid = false;
}

void mqtt_timezone_test_reset(void)
{
    s_timezone_topic[0] = '\0';
    s_last_payload[0] = '\0';
    s_last_payload_valid = false;
}
