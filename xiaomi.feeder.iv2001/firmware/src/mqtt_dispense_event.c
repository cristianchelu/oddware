/*
 * Dispense completion MQTT event — spec/30-processes/mqtt-protocol.md § Dispense event
 */

#include <stdio.h>
#include <string.h>

#include "app_log.h"
#include "dispense.h"
#include "feed_config.h"
#include "mqtt_dispense_event.h"
#include "mqtt_outbox.h"
#include "mqtt_port.h"
#include "mqtt_topics.h"
#include "port_err.h"

static char s_event_topic[96];

static const char *dispense_outcome_str(dispense_outcome_t outcome)
{
    switch (outcome) {
    case DISPENSE_OUTCOME_SUCCESS:
        return "success";
    case DISPENSE_OUTCOME_STUCK:
        return "stuck";
    case DISPENSE_OUTCOME_UNDERFILL:
        return "underfill";
    case DISPENSE_OUTCOME_EMPTY_HOPPER:
        return "empty_hopper";
    case DISPENSE_OUTCOME_ABORTED:
        return "aborted";
    default:
        return "success";
    }
}

static const char *dispense_source_str(dispense_source_t source)
{
    switch (source) {
    case DISPENSE_SOURCE_MQTT:
        return "mqtt";
    case DISPENSE_SOURCE_UART:
        return "uart";
    case DISPENSE_SOURCE_BUTTON:
        return "button";
    case DISPENSE_SOURCE_SCHEDULE:
        return "schedule";
    default:
        return "mqtt";
    }
}

static int mqtt_dispense_event_format_payload(const dispense_completion_t *completion,
                                                char *payload,
                                                size_t len)
{
    bool has_slot;
    const char *mode;
    int written;

    has_slot = completion->has_slot &&
               completion->source == DISPENSE_SOURCE_SCHEDULE;
    mode = feed_config_mode_string(completion->mode);

    if (has_slot) {
        written = snprintf(payload,
                           len,
                           "{\"event_type\":\"%s\","
                           "\"grams\":%ld,"
                           "\"grams_estimated\":%s,"
                           "\"target_g\":%u,"
                           "\"source\":\"%s\","
                           "\"mode\":\"%s\","
                           "\"batch_count\":%u,"
                           "\"slot_hour\":%u,"
                           "\"slot_min\":%u}",
                           dispense_outcome_str(completion->outcome),
                           (long)completion->grams,
                           completion->grams_estimated ? "true" : "false",
                           (unsigned)completion->target_g,
                           dispense_source_str(completion->source),
                           mode,
                           (unsigned)completion->batch_count,
                           (unsigned)completion->slot_hour,
                           (unsigned)completion->slot_min);
    } else {
        written = snprintf(payload,
                           len,
                           "{\"event_type\":\"%s\","
                           "\"grams\":%ld,"
                           "\"grams_estimated\":%s,"
                           "\"target_g\":%u,"
                           "\"source\":\"%s\","
                           "\"mode\":\"%s\","
                           "\"batch_count\":%u}",
                           dispense_outcome_str(completion->outcome),
                           (long)completion->grams,
                           completion->grams_estimated ? "true" : "false",
                           (unsigned)completion->target_g,
                           dispense_source_str(completion->source),
                           mode,
                           (unsigned)completion->batch_count);
    }

    return written;
}

void mqtt_dispense_event_set_device_id(const char *device_id)
{
    if (device_id == NULL || device_id[0] == '\0') {
        s_event_topic[0] = '\0';
        return;
    }

    if (mqtt_topic_format(s_event_topic,
                          sizeof(s_event_topic),
                          device_id,
                          "dispense/event")
            != PORT_OK) {
        s_event_topic[0] = '\0';
    }
}

bool mqtt_dispense_event_publish(const dispense_completion_t *completion)
{
    const mqtt_port_t *mqtt = mqtt_port_get();
    char payload[256];
    int written;

    if (completion == NULL || s_event_topic[0] == '\0') {
        return false;
    }

    if (mqtt == NULL || mqtt->is_connected == NULL || !mqtt->is_connected()) {
        return false;
    }

    written = mqtt_dispense_event_format_payload(completion, payload, sizeof(payload));
    if (written <= 0 || (size_t)written >= sizeof(payload)) {
        return false;
    }

    if (!mqtt_outbox_enqueue(s_event_topic, payload, (size_t)written, 1, false)) {
        app_log_debug("mqtt", "dispense event enqueue failed topic=%s", s_event_topic);
        return false;
    }

    app_log_info("mqtt",
                 "dispense event g=%ld est=%d outcome=%s",
                 (long)completion->grams,
                 completion->grams_estimated ? 1 : 0,
                 dispense_outcome_str(completion->outcome));
    return true;
}

void mqtt_dispense_event_test_reset(void)
{
    s_event_topic[0] = '\0';
}
