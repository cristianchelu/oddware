/*
 * Battery ADC sampling and MQTT sync — spec/30-processes/battery-monitoring.md
 */

#include "battery_monitor.h"

#include <stddef.h>

#include "adc_port.h"
#include "app_log.h"
#include "battery_pct.h"
#include "mqtt_battery.h"
#include "mqtt_battery_voltage.h"
#include "port_err.h"
#include "power_source_input.h"

#define BATTERY_MONITOR_INTERVAL_BATTERY_MS  60000u
#define BATTERY_MONITOR_INTERVAL_MAINS_MS    300000u

static uint32_t s_last_sample_ms;
static bool s_force_sample;
static bool s_has_last_sample_ms;

static uint32_t battery_monitor_interval_ms(void)
{
    if (!power_source_input_is_valid()) {
        return 0u;
    }

    if (power_source_input_get() == POWER_SOURCE_MAINS) {
        return BATTERY_MONITOR_INTERVAL_MAINS_MS;
    }

    return BATTERY_MONITOR_INTERVAL_BATTERY_MS;
}

static bool battery_monitor_due(uint32_t now_ms)
{
    uint32_t interval_ms;

    if (!power_source_input_is_valid()) {
        return false;
    }

    if (s_force_sample) {
        return true;
    }

    if (!s_has_last_sample_ms) {
        return true;
    }

    interval_ms = battery_monitor_interval_ms();
    if (interval_ms == 0u) {
        return false;
    }

    return (now_ms - s_last_sample_ms) >= interval_ms;
}

bool battery_monitor_poll(uint32_t now_ms)
{
    const adc_port_t *adc;
    uint16_t pack_mv;
    uint8_t pct;
    port_err_t err;
    bool force;

    if (!battery_monitor_due(now_ms)) {
        return false;
    }

    adc = adc_port_get();
    if (adc == NULL || adc->read_battery_mv == NULL) {
        return false;
    }

    err = adc->read_battery_mv(&pack_mv);
    if (err == PORT_ERR_BUSY) {
        return false;
    }

    if (err != PORT_OK) {
        app_log_debug("app", "battery adc read failed (%s)", port_err_name(err));
        return false;
    }

    force = s_force_sample;
    s_force_sample = false;
    s_last_sample_ms = now_ms;
    s_has_last_sample_ms = true;

    mqtt_battery_voltage_sync(pack_mv, force);

    if (pack_mv == 0u) {
        mqtt_battery_sync(false, 0u, force);
    } else {
        pct = battery_pct_from_mv(pack_mv, battery_pct_default_chemistry());
        mqtt_battery_sync(true, pct, force);
    }

    return true;
}

void battery_monitor_force_sample(void)
{
    s_force_sample = true;
}

void battery_monitor_test_reset(void)
{
    s_last_sample_ms = 0u;
    s_force_sample = false;
    s_has_last_sample_ms = false;
}
