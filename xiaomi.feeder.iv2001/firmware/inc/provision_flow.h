/*
 * Provisioning save + connect flow — spec/30-processes/provisioning-flow.md
 */

#ifndef PROVISION_FLOW_H
#define PROVISION_FLOW_H

#include <stdbool.h>
#include <stdint.h>

#include "config_port.h"
#include "provision_form.h"

typedef enum provision_flow_result {
    PROVISION_FLOW_OK = 0,
    PROVISION_FLOW_VALIDATION_FAIL,
    PROVISION_FLOW_WIFI_FAIL,
    PROVISION_FLOW_MQTT_WARN,
} provision_flow_result_t;

typedef struct provision_flow_ops {
    port_err_t (*save_wifi)(const config_port_t *cfg,
                            const char *ssid,
                            const char *pass);
    port_err_t (*save_mqtt)(const config_port_t *cfg,
                            const provision_input_t *input);
    bool (*wifi_try_connect)(const char *ssid, const char *pass, uint32_t timeout_ms);
    bool (*mqtt_try_connect)(const provision_input_t *input, uint32_t timeout_ms);
} provision_flow_ops_t;

provision_flow_result_t provision_flow_submit(const provision_input_t *input,
                                              const config_port_t *cfg,
                                              const provision_flow_ops_t *ops);

const char *provision_flow_message(provision_flow_result_t result);

#endif /* PROVISION_FLOW_H */
