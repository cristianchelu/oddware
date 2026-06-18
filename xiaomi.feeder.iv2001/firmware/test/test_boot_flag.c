/* Tests: spec/40-architecture/partition-layout.md A/B control block */

#include <string.h>

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
    TEST_ASSERT_TRUE(boot_bank_vector_table_valid(hdr, CM4_BASE));
}

void test_unverified_flag_detected(void)
{
    const boot_control_block_t ctrl = {
        .magic = BOOT_CTRL_MAGIC,
        .active_flag = BOOT_FLAG_A,
        .unverified = BOOT_UNVERIFIED_SET,
    };

    TEST_ASSERT_TRUE(boot_bank_is_unverified(&ctrl));
}

void test_erased_unverified_is_not_unverified(void)
{
    const boot_control_block_t ctrl = {
        .magic = BOOT_CTRL_MAGIC,
        .active_flag = BOOT_FLAG_A,
        .unverified = BOOT_FLAG_ERASED,
    };

    TEST_ASSERT_FALSE(boot_bank_is_unverified(&ctrl));
}

void test_arm_unverified_sets_flag_and_clears_attempts(void)
{
    boot_control_block_t ctrl = {
        .magic = BOOT_CTRL_MAGIC,
        .active_flag = BOOT_FLAG_A,
        .boot_attempts = 2,
    };

    boot_bank_arm_unverified(&ctrl);

    TEST_ASSERT_TRUE(boot_bank_is_unverified(&ctrl));
    TEST_ASSERT_EQUAL_UINT8(0, ctrl.boot_attempts);
}

void test_confirm_slot_clears_unverified_and_attempts(void)
{
    boot_control_block_t ctrl = {
        .magic = BOOT_CTRL_MAGIC,
        .active_flag = BOOT_FLAG_B,
        .unverified = BOOT_UNVERIFIED_SET,
        .boot_attempts = 2,
    };

    boot_bank_confirm_slot(&ctrl);

    TEST_ASSERT_FALSE(boot_bank_is_unverified(&ctrl));
    TEST_ASSERT_EQUAL_UINT8(0, ctrl.boot_attempts);
}

void test_boot_attempt_increments_while_unverified(void)
{
    boot_control_block_t ctrl = {
        .magic = BOOT_CTRL_MAGIC,
        .active_flag = BOOT_FLAG_A,
        .unverified = BOOT_UNVERIFIED_SET,
        .boot_attempts = 0,
    };

    TEST_ASSERT_EQUAL(BOOT_ATTEMPT_CONTINUE, boot_bank_record_boot_attempt(&ctrl));
    TEST_ASSERT_EQUAL_UINT8(1, ctrl.boot_attempts);
    TEST_ASSERT_EQUAL(BOOT_FLAG_A, ctrl.active_flag);
}

void test_boot_attempt_toggles_bank_at_limit(void)
{
    boot_control_block_t ctrl = {
        .magic = BOOT_CTRL_MAGIC,
        .active_flag = BOOT_FLAG_A,
        .unverified = BOOT_UNVERIFIED_SET,
        .boot_attempts = BOOT_MAX_ATTEMPTS - 1u,
    };

    TEST_ASSERT_EQUAL(BOOT_ATTEMPT_TOGGLED, boot_bank_record_boot_attempt(&ctrl));
    TEST_ASSERT_EQUAL_UINT8(0, ctrl.boot_attempts);
    TEST_ASSERT_EQUAL(BOOT_FLAG_B, ctrl.active_flag);
    TEST_ASSERT_TRUE(boot_bank_is_unverified(&ctrl));
}

void test_boot_attempt_ignored_when_verified(void)
{
    boot_control_block_t ctrl = {
        .magic = BOOT_CTRL_MAGIC,
        .active_flag = BOOT_FLAG_A,
        .unverified = BOOT_UNVERIFIED_CLEAR,
        .boot_attempts = 0,
    };

    TEST_ASSERT_EQUAL(BOOT_ATTEMPT_CONTINUE, boot_bank_record_boot_attempt(&ctrl));
    TEST_ASSERT_EQUAL_UINT8(0, ctrl.boot_attempts);
}

void test_erased_boot_attempts_treated_as_zero(void)
{
    boot_control_block_t ctrl = {
        .magic = BOOT_CTRL_MAGIC,
        .active_flag = BOOT_FLAG_A,
        .unverified = BOOT_UNVERIFIED_SET,
        .boot_attempts = 0xFF,
    };

    TEST_ASSERT_EQUAL_UINT8(0, boot_bank_effective_boot_attempts(0xFF));
    TEST_ASSERT_EQUAL(BOOT_ATTEMPT_CONTINUE, boot_bank_record_boot_attempt(&ctrl));
    TEST_ASSERT_EQUAL_UINT8(1, ctrl.boot_attempts);
    TEST_ASSERT_EQUAL(BOOT_FLAG_A, ctrl.active_flag);
}

static const uint8_t *s_scan_image;
static size_t s_scan_image_len;

static int test_scan_flash_read(uint32_t offset, uint8_t *buf, uint32_t len)
{
    if (s_scan_image == NULL || offset + len > s_scan_image_len) {
        return -1;
    }

    memcpy(buf, s_scan_image + offset, len);
    return 0;
}

void test_vector_scan_finds_table_in_first_64k(void)
{
    uint8_t image[0x108];
    const uint8_t vector[8] = {
        0x00, 0x00, 0x20, 0x14,
        0x01, 0x20, 0x01, 0x08,
    };

    memset(image, 0xFF, sizeof(image));
    memcpy(image + 0x100, vector, sizeof(vector));
    s_scan_image = image;
    s_scan_image_len = sizeof(image);

    TEST_ASSERT_TRUE(boot_bank_scan_vector_table(
        test_scan_flash_read,
        0,
        CM4_BASE));
}

void test_vector_scan_rejects_truncated_image(void)
{
    uint8_t image[8] = {0xD0, 0xF8, 0x74, 0xD0, 0x72, 0xB6, 0x21, 0xF0};

    s_scan_image = image;
    s_scan_image_len = sizeof(image);

    TEST_ASSERT_FALSE(boot_bank_scan_vector_table(
        test_scan_flash_read,
        0,
        CM4_BASE));
}
