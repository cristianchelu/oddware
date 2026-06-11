/*
 * Motor index port adapter accessor — host tests select real vs fake provider.
 */

#ifndef MOTOR_INDEX_PORT_ADAPTER_H
#define MOTOR_INDEX_PORT_ADAPTER_H

#include "motor_index_port.h"

const motor_index_port_t *motor_index_port_adapter_get(void);

#endif /* MOTOR_INDEX_PORT_ADAPTER_H */
