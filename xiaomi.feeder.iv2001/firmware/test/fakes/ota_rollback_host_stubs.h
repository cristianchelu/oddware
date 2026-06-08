#ifndef OTA_ROLLBACK_HOST_STUBS_H
#define OTA_ROLLBACK_HOST_STUBS_H

#include <stddef.h>

void ota_rollback_host_stub_reset(void);
size_t ota_rollback_host_stub_bank_switch_calls(void);
size_t ota_rollback_host_stub_reboot_calls(void);

#endif /* OTA_ROLLBACK_HOST_STUBS_H */
