/*
 * Target delay for display boot self-test — overrides weak display_boot_delay_ms.
 */

#include "hal_gpt.h"

#include "display_boot.h"

void display_boot_delay_ms(uint32_t ms)
{
    hal_gpt_delay_ms(ms);
}
