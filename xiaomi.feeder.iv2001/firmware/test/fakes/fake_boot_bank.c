#include <string.h>

#include "boot_bank_target.h"
#include "fake_boot_bank.h"

static boot_control_block_t s_ctrl = {
    .magic = BOOT_CTRL_MAGIC,
    .active_flag = BOOT_FLAG_A,
};
static size_t s_confirm_calls;

void fake_boot_bank_reset(void)
{
    memset(&s_ctrl, 0, sizeof(s_ctrl));
    s_ctrl.magic = BOOT_CTRL_MAGIC;
    s_ctrl.active_flag = BOOT_FLAG_A;
    s_ctrl.unverified = BOOT_UNVERIFIED_CLEAR;
    s_ctrl.boot_attempts = 0;
    s_confirm_calls = 0;
}

void fake_boot_bank_set_active(boot_bank_t bank)
{
    s_ctrl.active_flag = (bank == BOOT_BANK_B) ? BOOT_FLAG_B : BOOT_FLAG_A;
}

void fake_boot_bank_set_unverified(bool unverified)
{
    s_ctrl.unverified = unverified ? BOOT_UNVERIFIED_SET : BOOT_UNVERIFIED_CLEAR;
}

void fake_boot_bank_set_boot_attempts(uint8_t attempts)
{
    s_ctrl.boot_attempts = attempts;
}

size_t fake_boot_bank_confirm_calls(void)
{
    return s_confirm_calls;
}

boot_bank_t boot_bank_query_active(void)
{
    return boot_bank_resolve(&s_ctrl);
}

bool boot_bank_query_unverified(void)
{
    return boot_bank_is_unverified(&s_ctrl);
}

uint8_t boot_bank_query_boot_attempts(void)
{
    return boot_bank_effective_boot_attempts(s_ctrl.boot_attempts);
}

int boot_bank_switch_active(void)
{
    if (s_ctrl.active_flag == BOOT_FLAG_B) {
        s_ctrl.active_flag = BOOT_FLAG_A;
    } else {
        s_ctrl.active_flag = BOOT_FLAG_B;
    }

    boot_bank_arm_unverified(&s_ctrl);
    return 0;
}

int boot_bank_confirm_boot(void)
{
    boot_bank_confirm_slot(&s_ctrl);
    s_confirm_calls++;
    return 0;
}
