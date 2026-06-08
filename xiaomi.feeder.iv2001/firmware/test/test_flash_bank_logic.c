/* Tests: spec/30-processes/ota-flow.md, spec/40-architecture/partition-layout.md */

#include "unity.h"

#include "flash_bank_logic.h"
#include "boot_bank.h"

void test_inactive_bank_from_a(void)
{
    TEST_ASSERT_EQUAL(BOOT_BANK_B, flash_bank_inactive(BOOT_BANK_A));
}

void test_inactive_bank_from_b(void)
{
    TEST_ASSERT_EQUAL(BOOT_BANK_A, flash_bank_inactive(BOOT_BANK_B));
}

void test_bank_a_rom_offset(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x00012000, flash_bank_rom_offset(BOOT_BANK_A));
}

void test_bank_b_rom_offset(void)
{
    TEST_ASSERT_EQUAL_HEX32(0x00100000, flash_bank_rom_offset(BOOT_BANK_B));
}
