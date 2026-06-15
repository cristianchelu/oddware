/*
 * Host fake for app_cli OTA suspend — app_cli.c is ARM-only (UART HAL).
 * Production ota_preflight.c links this instead; same call sequence as device.
 */

#include <stdbool.h>

#include "app_cli_ota.h"

static bool s_suspended_for_ota;

void app_cli_test_reset(void)
{
    s_suspended_for_ota = false;
}

bool app_cli_test_is_suspended_for_ota(void)
{
    return s_suspended_for_ota;
}

void app_cli_suspend_for_ota(void)
{
    s_suspended_for_ota = true;
}

void app_cli_resume_after_ota(void)
{
    s_suspended_for_ota = false;
}
