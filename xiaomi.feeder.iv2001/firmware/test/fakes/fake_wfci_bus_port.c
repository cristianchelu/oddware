#include "fake_wfci_bus_port.h"

#include <string.h>

#include "fake_time.h"
#include "wfci_bus_state.h"

#define FAKE_WFCI_MAX_RECORDS  32u
#define FAKE_WFCI_DEFER_NONE   UINT32_MAX

static fake_wfci_bus_record_t s_acquires[FAKE_WFCI_MAX_RECORDS];
static size_t s_acquire_count;
static size_t s_release_count;
static uint32_t s_max_hold_ms;
static uint32_t s_hold_start_ms;
static bool s_held;
static wfci_bus_profile_t s_held_profile;
static port_err_t s_acquire_err = PORT_OK;
static port_err_t s_try_acquire_err = PORT_ERR_BUSY;
static uint32_t s_deferred_release_ms = FAKE_WFCI_DEFER_NONE;

static void fake_release_internal(wfci_bus_profile_t profile)
{
    uint32_t held_ms;

    if (!s_held || profile != s_held_profile) {
        return;
    }

    held_ms = (uint32_t)fake_time_ticks() - s_hold_start_ms;
    if (s_acquire_count > 0u) {
        s_acquires[s_acquire_count - 1u].hold_ms = held_ms;
    }
    if (held_ms > s_max_hold_ms) {
        s_max_hold_ms = held_ms;
    }

    s_held = false;
    s_deferred_release_ms = FAKE_WFCI_DEFER_NONE;
    wfci_bus_state_set_held(profile, false);
    s_release_count++;
}

static void fake_maybe_deferred_release(void)
{
    if (!s_held || s_deferred_release_ms == FAKE_WFCI_DEFER_NONE) {
        return;
    }

    if (fake_time_ticks() >= (s_hold_start_ms + s_deferred_release_ms)) {
        fake_release_internal(s_held_profile);
    }
}

static port_err_t fake_grant_acquire(wfci_bus_profile_t profile,
                                     wfci_bus_priority_t priority)
{
    if (s_acquire_err != PORT_OK) {
        return s_acquire_err;
    }

    if (s_acquire_count < FAKE_WFCI_MAX_RECORDS) {
        s_acquires[s_acquire_count].profile = profile;
        s_acquires[s_acquire_count].priority = priority;
        s_acquires[s_acquire_count].hold_ms = 0u;
        s_acquire_count++;
    }

    s_held = true;
    s_held_profile = profile;
    s_hold_start_ms = (uint32_t)fake_time_ticks();
    wfci_bus_state_set_held(profile, true);
    return PORT_OK;
}

static port_err_t fake_acquire(wfci_bus_profile_t profile,
                               wfci_bus_priority_t priority,
                               uint32_t timeout_ms)
{
    if (s_held) {
        if (timeout_ms == 0u) {
            return PORT_ERR_BUSY;
        }

        {
            TickType_t start = fake_time_ticks();

            while (s_held) {
                fake_maybe_deferred_release();
                if (!s_held) {
                    break;
                }
                if ((fake_time_ticks() - start) >= (TickType_t)timeout_ms) {
                    return PORT_ERR_BUSY;
                }
                fake_time_advance_ms(1u);
            }
        }
    }

    return fake_grant_acquire(profile, priority);
}

static port_err_t fake_try_acquire(wfci_bus_profile_t profile,
                                   wfci_bus_priority_t priority)
{
    if (s_held) {
        return s_try_acquire_err;
    }

    return fake_grant_acquire(profile, priority);
}

static void fake_release(wfci_bus_profile_t profile)
{
    fake_release_internal(profile);
}

static const wfci_bus_port_t s_fake_bus = {
    .acquire = fake_acquire,
    .try_acquire = fake_try_acquire,
    .release = fake_release,
};

void fake_wfci_bus_reset(void)
{
    memset(s_acquires, 0, sizeof(s_acquires));
    s_acquire_count = 0u;
    s_release_count = 0u;
    s_max_hold_ms = 0u;
    s_hold_start_ms = 0u;
    s_held = false;
    s_acquire_err = PORT_OK;
    s_try_acquire_err = PORT_ERR_BUSY;
    s_deferred_release_ms = FAKE_WFCI_DEFER_NONE;
    wfci_bus_state_set_held(WFCI_BUS_PROFILE_EXPANDER, false);
}

void fake_wfci_bus_arm_release_after_ms(uint32_t ms_from_hold_start)
{
    s_deferred_release_ms = ms_from_hold_start;
}

const fake_wfci_bus_record_t *fake_wfci_bus_acquires(size_t *count)
{
    if (count != NULL) {
        *count = s_acquire_count;
    }
    return s_acquires;
}

size_t fake_wfci_bus_release_count(void)
{
    return s_release_count;
}

uint32_t fake_wfci_bus_max_hold_ms(void)
{
    return s_max_hold_ms;
}

void fake_wfci_bus_set_acquire_err(port_err_t err)
{
    s_acquire_err = err;
}

void fake_wfci_bus_set_try_acquire_err(port_err_t err)
{
    s_try_acquire_err = err;
}

const wfci_bus_port_t *fake_wfci_bus_port_get(void)
{
    return &s_fake_bus;
}

const wfci_bus_port_t *wfci_bus_port_get(void)
{
    return fake_wfci_bus_port_get();
}
