/*
 * WFCI bus held-profile state — shared by adapter and host fakes.
 */

#include "wfci_bus_state.h"

static bool s_loan_held;
static wfci_bus_profile_t s_held_profile;

static bool profile_covers_expander(wfci_bus_profile_t profile)
{
    return profile == WFCI_BUS_PROFILE_EXPANDER
        || profile == WFCI_BUS_PROFILE_DISPLAY
        || profile == WFCI_BUS_PROFILE_WEIGH
        || profile == WFCI_BUS_PROFILE_FULL;
}

void wfci_bus_state_set_held(wfci_bus_profile_t profile, bool held)
{
    s_loan_held = held;
    if (held) {
        s_held_profile = profile;
    }
}

bool wfci_bus_expander_accessible(void)
{
    return s_loan_held && profile_covers_expander(s_held_profile);
}
