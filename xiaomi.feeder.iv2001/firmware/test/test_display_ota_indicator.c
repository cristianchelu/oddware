/* Tests: spec/30-processes/display-presentation.md § OTA indicator */

#include "unity.h"

#include "display_ota_indicator.h"
#include "display_presentation.h"
#include "fake_display_port.h"

static void ota_indicator_test_setup(void)
{
    fake_display_port_reset();
    display_presentation_reset();
}

void test_display_ota_indicator_start_connecting_blink(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    ota_indicator_test_setup();
    display_ota_indicator_start();
    TEST_ASSERT_TRUE(display_presentation_ota_active());
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_refresh());

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x40u, grids[0]);
}

void test_display_ota_indicator_progress_preparing_connecting(void)
{
    ota_progress_t progress = {
        .status = OTA_STATUS_PREPARING,
        .pct = 0u,
        .error = "",
    };

    ota_indicator_test_setup();
    display_ota_indicator_on_progress(&progress);
    TEST_ASSERT_TRUE(display_presentation_ota_active());

    progress.status = OTA_STATUS_CONNECTING;
    display_ota_indicator_on_progress(&progress);
    TEST_ASSERT_TRUE(display_presentation_ota_active());
}

void test_display_ota_indicator_progress_downloading_pct45(void)
{
    uint8_t grids[TM1637_GRID_COUNT];
    ota_progress_t progress = {
        .status = OTA_STATUS_DOWNLOADING,
        .pct = 45u,
        .error = "",
    };

    ota_indicator_test_setup();
    display_ota_indicator_on_progress(&progress);
    TEST_ASSERT_EQUAL(PORT_OK, display_presentation_refresh());

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x07u, grids[2]);
}

void test_display_ota_indicator_progress_verifying_then_apply_clears(void)
{
    ota_progress_t progress = {
        .status = OTA_STATUS_VERIFYING,
        .pct = 100u,
        .error = "",
    };

    ota_indicator_test_setup();
    display_ota_indicator_on_progress(&progress);
    TEST_ASSERT_TRUE(display_presentation_ota_active());

    progress.status = OTA_STATUS_APPLYING;
    display_ota_indicator_on_progress(&progress);
    TEST_ASSERT_FALSE(display_presentation_ota_active());
}

void test_display_ota_indicator_error_stops_override(void)
{
    ota_progress_t progress = {
        .status = OTA_STATUS_ERROR,
        .pct = 0u,
        .error = "download_failed",
    };

    ota_indicator_test_setup();
    display_ota_indicator_start();
    display_ota_indicator_on_progress(&progress);
    TEST_ASSERT_FALSE(display_presentation_ota_active());
}
