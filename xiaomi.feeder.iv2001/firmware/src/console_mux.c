/*
 * Console transport mux — spec/30-processes/uart-console.md § Remote telnet console
 */

#include "console_mux.h"

static bool s_remote_active;
static bool s_force_local_req;

void console_mux_init(void)
{
    s_remote_active = false;
    s_force_local_req = false;
}

bool console_mux_try_remote(void)
{
    if (s_remote_active) {
        return false;
    }

    s_remote_active = true;
    s_force_local_req = false;
    return true;
}

void console_mux_release_remote(void)
{
    s_remote_active = false;
    s_force_local_req = false;
}

bool console_mux_remote_active(void)
{
    return s_remote_active;
}

void console_mux_request_force_local(void)
{
    if (s_remote_active) {
        s_force_local_req = true;
    }
}

bool console_mux_force_local_pending(void)
{
    return s_force_local_req;
}

bool console_mux_take_force_local(void)
{
    if (!s_force_local_req) {
        return false;
    }

    s_force_local_req = false;
    return true;
}

#ifdef HOST_TEST
void console_mux_test_reset(void)
{
    console_mux_init();
}
#endif
