/*
 * Slot health confirm after bank swap — spec/30-processes/ota-flow.md
 */

#ifndef OTA_SLOT_HEALTH_H
#define OTA_SLOT_HEALTH_H

#include <stdint.h>

void ota_slot_health_on_boot(void);
uint32_t ota_slot_health_poll_ms(void);

#endif /* OTA_SLOT_HEALTH_H */
