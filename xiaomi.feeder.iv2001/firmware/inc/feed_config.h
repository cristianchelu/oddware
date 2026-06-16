/*
 * Feed settings in NVDM — spec/30-processes/config-store.md
 */

#ifndef FEED_CONFIG_H
#define FEED_CONFIG_H

#include <stdbool.h>

bool feed_config_child_lock_is_active(void);
bool feed_config_child_lock_set(bool locked);
bool feed_config_child_lock_toggle(void);

#endif /* FEED_CONFIG_H */
