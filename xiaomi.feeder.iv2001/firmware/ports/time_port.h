/*
 * Time port — spec/40-architecture/ports.md
 */

#ifndef TIME_PORT_H
#define TIME_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "port_err.h"

typedef struct time_port {
    port_err_t (*init)(void);
    port_err_t (*get_utc_epoch)(int64_t *epoch_out);
    port_err_t (*request_sync)(void);
    port_err_t (*poll_sync)(bool *done_out, bool *ok_out, int64_t *epoch_out);
} time_port_t;

const time_port_t *time_port_get(void);

#endif /* TIME_PORT_H */
