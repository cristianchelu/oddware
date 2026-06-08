#ifndef FAKE_MQTT_PORT_H
#define FAKE_MQTT_PORT_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#include "mqtt_port.h"

typedef struct fake_mqtt_port_state {
    unsigned connect_calls;
    unsigned disconnect_calls;
    unsigned subscribe_calls;
    unsigned publish_calls;
    char last_subscribe_topic[128];
    char last_publish_topic[128];
    char last_publish_payload[128];
    char prior_publish_topic[128];
    char prior_publish_payload[128];
    bool fail_next_connect;
    bool fail_subscribe;
    bool fail_publish;
    bool connected;
} fake_mqtt_port_state_t;

void fake_mqtt_port_reset(void);
void fake_mqtt_port_set_fail_next_connect(bool fail);
const mqtt_port_t *fake_mqtt_port_get(void);
const fake_mqtt_port_state_t *fake_mqtt_port_state(void);

#endif /* FAKE_MQTT_PORT_H */
