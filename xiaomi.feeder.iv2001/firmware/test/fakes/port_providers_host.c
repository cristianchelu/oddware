#include "config_port.h"
#include "fake_config_port.h"
#include "fake_mqtt_port.h"
#include "fake_ota_port.h"
#include "fake_wifi_port.h"
#include "mqtt_port.h"
#include "ota_port.h"
#include "wifi_port.h"

const config_port_t *config_port_get(void)
{
    return fake_config_port_get();
}

const mqtt_port_t *mqtt_port_get(void)
{
    return fake_mqtt_port_get();
}

const wifi_port_t *wifi_port_get(void)
{
    return fake_wifi_port_get();
}

const ota_port_t *ota_port_get(void)
{
    return fake_ota_port_get();
}
