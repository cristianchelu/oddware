/* Tests: spec/40-architecture/partition-layout.md A/B control block */

#include "unity.h"
#include "boot_bank.h"

void test_flag_a_selects_bank_a(void)
{
    const boot_control_block_t ctrl = {
        .magic = BOOT_CTRL_MAGIC,
        .active_flag = BOOT_FLAG_A,
    };

    TEST_ASSERT_EQUAL(BOOT_BANK_A, boot_bank_resolve(&ctrl));
    TEST_ASSERT_EQUAL_HEX32(CM4_BASE, boot_bank_load_address(BOOT_BANK_A));
}

void test_flag_b_selects_bank_b(void)
{
    const boot_control_block_t ctrl = {
        .magic = BOOT_CTRL_MAGIC,
        .active_flag = BOOT_FLAG_B,
    };

    TEST_ASSERT_EQUAL(BOOT_BANK_B, boot_bank_resolve(&ctrl));
    TEST_ASSERT_EQUAL_HEX32(BANK_B_BASE, boot_bank_load_address(BOOT_BANK_B));
}

void test_erased_flag_defaults_to_bank_a(void)
{
    const boot_control_block_t ctrl = {
        .magic = BOOT_CTRL_MAGIC,
        .active_flag = BOOT_FLAG_ERASED,
    };

    TEST_ASSERT_EQUAL(BOOT_BANK_A, boot_bank_resolve(&ctrl));
}

void test_corrupt_flag_defaults_to_bank_a(void)
{
    const boot_control_block_t ctrl = {
        .magic = BOOT_CTRL_MAGIC,
        .active_flag = 0xDEADBEEFu,
    };

    TEST_ASSERT_EQUAL(BOOT_BANK_A, boot_bank_resolve(&ctrl));
}

void test_invalid_magic_defaults_to_bank_a(void)
{
    const boot_control_block_t ctrl = {
        .magic = 0u,
        .active_flag = BOOT_FLAG_B,
    };

    TEST_ASSERT_EQUAL(BOOT_BANK_A, boot_bank_resolve(&ctrl));
}

void test_null_ctrl_defaults_to_bank_a(void)
{
    TEST_ASSERT_EQUAL(BOOT_BANK_A, boot_bank_resolve(NULL));
}

void test_image_header_rejects_erased(void)
{
    const uint8_t hdr[8] = {0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF};

    TEST_ASSERT_FALSE(boot_bank_image_header_valid(hdr, CM4_BASE));
}

void test_image_header_accepts_reset_handler(void)
{
    const uint8_t hdr[8] = {0xD0, 0xF8, 0x74, 0xD0, 0x72, 0xB6, 0x21, 0xF0};

    TEST_ASSERT_TRUE(boot_bank_image_header_valid(hdr, CM4_BASE));
}

void test_image_header_accepts_vector_table(void)
{
    const uint8_t hdr[8] = {
        0x00, 0x00, 0x20, 0x14,
        0x01, 0x20, 0x01, 0x08,
    };

    TEST_ASSERT_TRUE(boot_bank_image_header_valid(hdr, CM4_BASE));
}
