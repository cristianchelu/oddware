/*
 * Motor index broken-beam port — spec/40-architecture/ports.md
 */

#ifndef MOTOR_INDEX_PORT_H
#define MOTOR_INDEX_PORT_H

#include <stdbool.h>

#include "port_err.h"

typedef struct motor_index_port {
    port_err_t (*set_led)(bool on);
    port_err_t (*read_beam_open)(bool *beam_open);
} motor_index_port_t;

const motor_index_port_t *motor_index_port_get(void);

#endif /* MOTOR_INDEX_PORT_H */
