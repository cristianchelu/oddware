#ifndef MQTT_ADAPTER_H
#define MQTT_ADAPTER_H

#include <stdbool.h>
#include <stdint.h>

#include "provision_form.h"

void mqtt_adapter_yield(int timeout_ms);

bool mqtt_adapter_probe_broker(const provision_input_t *input,
                               const char *mac_hex12,
                               uint32_t timeout_ms);

#endif /* MQTT_ADAPTER_H */
