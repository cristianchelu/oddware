#ifndef FAKE_WEIGHT_PORT_H
#define FAKE_WEIGHT_PORT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "weight_port.h"

typedef enum {
    FAKE_WEIGHT_OP_POWER_ON = 0,
    FAKE_WEIGHT_OP_POWER_OFF,
    FAKE_WEIGHT_OP_READ_DG,
    FAKE_WEIGHT_OP_TRY_READ_DG,
    FAKE_WEIGHT_OP_CAL_ZERO,
    FAKE_WEIGHT_OP_CAL_SPAN,
} fake_weight_op_kind_t;

typedef struct fake_weight_op {
    fake_weight_op_kind_t kind;
    weight_dg_t dg;
} fake_weight_op_t;

void fake_weight_port_reset(void);
void fake_weight_port_set_read_dg(weight_dg_t dg);
void fake_weight_port_set_read_err(port_err_t err);
void fake_weight_port_set_cal_status(weight_cal_status_t st);
void fake_weight_port_set_scale_off(bool off);
const fake_weight_op_t *fake_weight_port_ops(size_t *count);
const weight_port_t *fake_weight_port_get(void);

#endif /* FAKE_WEIGHT_PORT_H */
