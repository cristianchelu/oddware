#ifndef HAL_SYS_H
#define HAL_SYS_H

#include <stdint.h>

#define HAL_SYS_REBOOT_MAGIC 0
#define WHOLE_SYSTEM_REBOOT_COMMAND 0

void hal_sys_reboot(uint32_t magic, uint32_t command);

#endif /* HAL_SYS_H */
