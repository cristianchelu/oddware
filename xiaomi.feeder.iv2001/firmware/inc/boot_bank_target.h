/*
 * Target flash helpers for A/B control block (not host-tested).
 */

#ifndef __BOOT_BANK_TARGET_H__
#define __BOOT_BANK_TARGET_H__

#include "boot_bank.h"

boot_bank_t boot_bank_query_active(void);
int boot_bank_switch_active(void);

#endif /* __BOOT_BANK_TARGET_H__ */
