/*
 * Feed settings in NVDM — spec/30-processes/config-store.md
 */

#ifndef FEED_CONFIG_H
#define FEED_CONFIG_H

#include <stdbool.h>

#include "dispense.h"

bool feed_config_child_lock_is_active(void);
bool feed_config_child_lock_set(bool locked);
bool feed_config_child_lock_toggle(void);

dispense_mode_t feed_config_mode_get(void);
bool feed_config_mode_set(dispense_mode_t mode);
const char *feed_config_mode_string(dispense_mode_t mode);

#define FEED_OVERFILL_THRESHOLD_G_MIN  30u
#define FEED_OVERFILL_THRESHOLD_G_MAX  100u
#define FEED_OVERFILL_THRESHOLD_G_DEFAULT 50u

bool feed_config_overfill_enabled_get(void);
bool feed_config_overfill_enabled_set(bool enabled);
uint8_t feed_config_overfill_threshold_g_get(void);
bool feed_config_overfill_threshold_g_set(uint8_t threshold_g);

#endif /* FEED_CONFIG_H */
