/* Tests: spec/30-processes/ota-flow.md */

#include "unity.h"

#include "memory_map.h"
#include "ota_image.h"

void test_progress_pct_rounds_down_to_step(void)
{
    TEST_ASSERT_EQUAL_UINT8(0, ota_progress_pct(0, 1000));
    TEST_ASSERT_EQUAL_UINT8(5, ota_progress_pct(50, 1000));
    TEST_ASSERT_EQUAL_UINT8(10, ota_progress_pct(100, 1000));
    TEST_ASSERT_EQUAL_UINT8(100, ota_progress_pct(1000, 1000));
}

void test_image_size_allowed_within_bank(void)
{
    TEST_ASSERT_TRUE(ota_image_size_allowed(1024));
    TEST_ASSERT_TRUE(ota_image_size_allowed(CM4_LENGTH));
    TEST_ASSERT_FALSE(ota_image_size_allowed(0));
    TEST_ASSERT_FALSE(ota_image_size_allowed(CM4_LENGTH + 1));
}

void test_vector_table_accepts_valid_header(void)
{
    uint8_t image[8] = {
        0x00, 0x00, 0x20, 0x14,
        0x01, 0x20, 0x01, 0x08,
    };

    TEST_ASSERT_EQUAL(PORT_OK, ota_image_check_vector_table(image, sizeof(image)));
}

void test_vector_table_rejects_bad_stack(void)
{
    uint8_t image[8] = {
        0x00, 0x00, 0x00, 0x00,
        0x09, 0x00, 0x01, 0x08,
    };

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, ota_image_check_vector_table(image, sizeof(image)));
}

void test_vector_table_accepts_bank_b_reset(void)
{
    uint8_t image[8] = {
        0x00, 0x00, 0x20, 0x14,
        0x09, 0x00, 0x10, 0x08,
    };

    TEST_ASSERT_EQUAL(PORT_OK,
                       ota_image_check_vector_table_at(BANK_B_BASE, image, sizeof(image)));
}
