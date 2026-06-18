/*
 * Battery ADC sampling and MQTT sync — spec/30-processes/battery-monitoring.md
 */

#ifndef BATTERY_MONITOR_H
#define BATTERY_MONITOR_H

#include <stdbool.h>
#include <stdint.h>

bool battery_monitor_poll(uint32_t now_ms);
void battery_monitor_force_sample(void);

void battery_monitor_test_reset(void);

#endif /* BATTERY_MONITOR_H */
