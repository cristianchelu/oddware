/*
 * Provisioning save + connect flow — spec/30-processes/provisioning-flow.md
 */

#include "provision_flow.h"

#include "mqtt_cred.h"
#include "wifi_cred.h"

static port_err_t provision_flow_save_mqtt_keys(const config_port_t *cfg,
                                                const provision_input_t *input)
{
    port_err_t err;
    uint16_t port = input->mqtt_port_set ? input->mqtt_port : 1883;

    if (cfg == NULL || input == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = mqtt_cred_save_host(cfg, input->mqtt_host);
    if (err != PORT_OK) {
        return err;
    }

    err = mqtt_cred_save_port(cfg, port);
    if (err != PORT_OK) {
        return err;
    }

    err = mqtt_cred_save_user(cfg, input->mqtt_user);
    if (err != PORT_OK) {
        return err;
    }

    err = mqtt_cred_save_pass(cfg, input->mqtt_pass);
    if (err != PORT_OK) {
        return err;
    }

    return mqtt_cred_save_device_id(cfg, input->device_id);
}

provision_flow_result_t provision_flow_submit(const provision_input_t *input,
                                              const config_port_t *cfg,
                                              const provision_flow_ops_t *ops)
{
    bool wifi_ok;
    bool mqtt_ok;

    if (input == NULL || cfg == NULL || ops == NULL) {
        return PROVISION_FLOW_VALIDATION_FAIL;
    }

    if (provision_form_validate(input) != PORT_OK) {
        return PROVISION_FLOW_VALIDATION_FAIL;
    }

    if (ops->wifi_try_connect == NULL) {
        return PROVISION_FLOW_WIFI_FAIL;
    }

    wifi_ok = ops->wifi_try_connect(input->wifi_ssid, input->wifi_pass, 15000);
    if (!wifi_ok) {
        return PROVISION_FLOW_WIFI_FAIL;
    }

    if (ops->save_mqtt != NULL) {
        if (ops->save_mqtt(cfg, input) != PORT_OK) {
            return PROVISION_FLOW_WIFI_FAIL;
        }
    } else if (provision_flow_save_mqtt_keys(cfg, input) != PORT_OK) {
        return PROVISION_FLOW_WIFI_FAIL;
    }

    mqtt_ok = true;
    if (ops->mqtt_try_connect != NULL) {
        mqtt_ok = ops->mqtt_try_connect(input, 10000);
    }

    if (ops->save_wifi != NULL) {
        if (ops->save_wifi(cfg, input->wifi_ssid, input->wifi_pass) != PORT_OK) {
            return PROVISION_FLOW_WIFI_FAIL;
        }
    } else if (wifi_cred_save(cfg, input->wifi_ssid, input->wifi_pass) != PORT_OK) {
        return PROVISION_FLOW_WIFI_FAIL;
    }

    if (!mqtt_ok) {
        return PROVISION_FLOW_MQTT_WARN;
    }

    return PROVISION_FLOW_OK;
}

const char *provision_flow_message(provision_flow_result_t result)
{
    switch (result) {
    case PROVISION_FLOW_VALIDATION_FAIL:
        return PROVISION_MSG_VALIDATION;
    case PROVISION_FLOW_WIFI_FAIL:
        return PROVISION_MSG_WIFI_FAIL;
    case PROVISION_FLOW_MQTT_WARN:
        return PROVISION_MSG_MQTT_WARN;
    case PROVISION_FLOW_OK:
        return PROVISION_MSG_SUCCESS;
    default:
        return PROVISION_MSG_VALIDATION;
    }
}
