/*
 * MQTT publish outbox — spec/30-processes/mqtt-protocol.md § Publish path
 */

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "app_log.h"
#include "mqtt_outbox.h"

#define MQTT_OUTBOX_DEPTH          20u
#define MQTT_OUTBOX_MAX_PAYLOAD    768u
#define MQTT_OUTBOX_MAX_TOPIC      128u
#define MQTT_OUTBOX_MIN_DRAIN_MS   100u

typedef struct {
    char topic[MQTT_OUTBOX_MAX_TOPIC];
    uint8_t payload[MQTT_OUTBOX_MAX_PAYLOAD];
    size_t payload_len;
    uint8_t qos;
    bool retain;
} mqtt_outbox_slot_t;

static mqtt_outbox_slot_t s_slots[MQTT_OUTBOX_DEPTH];
static unsigned s_head;
static unsigned s_count;
static TickType_t s_last_drain_ticks;
static bool s_drained_once;
static mqtt_outbox_drained_fn s_drained_fn;
static void *s_drained_ctx;
static bool s_accepting = true;

void mqtt_outbox_set_drained_fn(mqtt_outbox_drained_fn fn, void *ctx)
{
    s_drained_fn = fn;
    s_drained_ctx = ctx;
}

void mqtt_outbox_set_accepting(bool accepting)
{
    s_accepting = accepting;
}

bool mqtt_outbox_is_accepting(void)
{
    return s_accepting;
}

void mqtt_outbox_reset(void)
{
    memset(s_slots, 0, sizeof(s_slots));
    s_head = 0;
    s_count = 0;
    s_last_drain_ticks = 0;
    s_drained_once = false;
}

unsigned mqtt_outbox_pending(void)
{
    return s_count;
}

bool mqtt_outbox_enqueue(const char *topic,
                         const void *payload,
                         size_t len,
                         uint8_t qos,
                         bool retain)
{
    unsigned tail;
    mqtt_outbox_slot_t *slot;

    if (topic == NULL || topic[0] == '\0') {
        return false;
    }
    if (payload == NULL && len != 0u) {
        return false;
    }
    if (len >= MQTT_OUTBOX_MAX_PAYLOAD) {
        return false;
    }
    if (strlen(topic) >= MQTT_OUTBOX_MAX_TOPIC) {
        return false;
    }
    if (!s_accepting) {
        return false;
    }
    if (s_count >= MQTT_OUTBOX_DEPTH) {
        app_log_debug("mqtt", "outbox full drop topic=%s", topic);
        return false;
    }

    tail = (s_head + s_count) % MQTT_OUTBOX_DEPTH;
    slot = &s_slots[tail];
    strncpy(slot->topic, topic, sizeof(slot->topic) - 1);
    if (len > 0u) {
        memcpy(slot->payload, payload, len);
    } else {
        slot->payload[0] = '\0';
    }
    slot->payload_len = len;
    slot->qos = qos;
    slot->retain = retain;
    s_count++;
    return true;
}

bool mqtt_outbox_drain_one(const mqtt_port_t *mqtt)
{
    mqtt_outbox_slot_t *slot;
    TickType_t now;
    uint32_t elapsed_ms;

    if (mqtt == NULL || s_count == 0) {
        return false;
    }

    now = xTaskGetTickCount();
    if (s_drained_once) {
        elapsed_ms = (uint32_t)(now - s_last_drain_ticks);
        if (elapsed_ms < MQTT_OUTBOX_MIN_DRAIN_MS) {
            return false;
        }
    }

    if (!mqtt->is_connected()) {
        return false;
    }

    slot = &s_slots[s_head];
    if (mqtt->publish(slot->topic,
                      slot->payload_len > 0u ? slot->payload : NULL,
                      slot->payload_len,
                      slot->qos,
                      slot->retain) != PORT_OK) {
        return false;
    }

    if (s_drained_fn != NULL) {
        s_drained_fn(slot->topic, slot->payload, slot->payload_len, s_drained_ctx);
    }

    s_last_drain_ticks = now;
    s_drained_once = true;
    s_head = (s_head + 1u) % MQTT_OUTBOX_DEPTH;
    s_count--;
    return true;
}
