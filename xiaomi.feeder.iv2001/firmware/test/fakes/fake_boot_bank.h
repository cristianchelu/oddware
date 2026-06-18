/*
 * Host fake for A/B control block — slot health tests.
 */

#ifndef FAKE_BOOT_BANK_H
#define FAKE_BOOT_BANK_H

#include <stdbool.h>
#include "boot_bank.h"

void fake_boot_bank_reset(void);
void fake_boot_bank_set_active(boot_bank_t bank);
void fake_boot_bank_set_unverified(bool unverified);
void fake_boot_bank_set_boot_attempts(uint8_t attempts);
size_t fake_boot_bank_confirm_calls(void);

#endif /* FAKE_BOOT_BANK_H */
