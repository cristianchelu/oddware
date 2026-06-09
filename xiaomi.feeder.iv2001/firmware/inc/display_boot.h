/*
 * Boot display self-test — spec/30-processes/display-presentation.md
 */

#ifndef DISPLAY_BOOT_H
#define DISPLAY_BOOT_H

#include <stdint.h>

#include "port_err.h"

#define DISPLAY_BOOT_PRE_POWER_MS   50u
#define DISPLAY_BOOT_LIGHT_TEST_MS  1000u

port_err_t display_boot_run(void);
void display_boot_delay_ms(uint32_t ms);

#endif /* DISPLAY_BOOT_H */
