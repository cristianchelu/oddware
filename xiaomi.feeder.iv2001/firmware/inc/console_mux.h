#ifndef CONSOLE_MUX_H
#define CONSOLE_MUX_H

#include <stdbool.h>

void console_mux_init(void);
bool console_mux_try_remote(void);
void console_mux_release_remote(void);
bool console_mux_remote_active(void);
void console_mux_request_force_local(void);
bool console_mux_force_local_pending(void);
bool console_mux_take_force_local(void);

#ifdef HOST_TEST
void console_mux_test_reset(void);
#endif

#endif /* CONSOLE_MUX_H */
