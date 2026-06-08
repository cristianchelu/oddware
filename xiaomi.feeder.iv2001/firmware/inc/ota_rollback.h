/*
 * OTA rollback after failed post-update boot — spec/30-processes/ota-flow.md
 */

#ifndef OTA_ROLLBACK_H
#define OTA_ROLLBACK_H

#include <stdint.h>

void ota_rollback_on_boot(void);
void ota_rollback_on_mqtt_connected(void);
uint32_t ota_rollback_poll_ms(void);
void ota_rollback_mark_pending(void);

#endif /* OTA_ROLLBACK_H */
