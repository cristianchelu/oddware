#include "boot_bank_target.h"
#include "hal_cache.h"
#include "hal_sys.h"
#include "ota_rollback_host_stubs.h"

static size_t s_bank_switch_calls;
static size_t s_reboot_calls;

void ota_rollback_host_stub_reset(void)
{
    s_bank_switch_calls = 0;
    s_reboot_calls = 0;
}

size_t ota_rollback_host_stub_bank_switch_calls(void)
{
    return s_bank_switch_calls;
}

size_t ota_rollback_host_stub_reboot_calls(void)
{
    return s_reboot_calls;
}

int boot_bank_switch_active(void)
{
    s_bank_switch_calls++;
    return 0;
}

void hal_cache_disable(void)
{
}

void hal_cache_deinit(void)
{
}

void hal_sys_reboot(uint32_t magic, uint32_t command)
{
    (void)magic;
    (void)command;
    s_reboot_calls++;
}
