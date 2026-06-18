/*
 * Target flash helpers for A/B control block (not host-tested).
 */

#ifndef __BOOT_BANK_TARGET_H__
#define __BOOT_BANK_TARGET_H__

#include "boot_bank.h"

boot_bank_t boot_bank_query_active(void);
bool boot_bank_query_unverified(void);
uint8_t boot_bank_query_boot_attempts(void);
int boot_bank_switch_active(void);
int boot_bank_switch_with_hash(const uint8_t image_hash[64]);
int boot_bank_confirm_boot(void);

#endif /* __BOOT_BANK_TARGET_H__ */
