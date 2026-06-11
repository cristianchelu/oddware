/*
 * Motor jam / burst tuning — spec/30-processes/jam-detection.md,
 * spec/30-processes/dispense-cycle.md, spec/30-processes/motor-index.md
 */

#ifndef MOTOR_JAM_H
#define MOTOR_JAM_H

#include <stdint.h>

#define MOTOR_JAM_INSTANT_MA           1800u
#define MOTOR_JAM_SUSTAINED_MA          500u
#define MOTOR_JAM_SUSTAINED_MS         4000u
#define MOTOR_BURST_TIMEOUT_MS         8000u
#define MOTOR_BURST_PULSE_DEFAULT       1u
#define MOTOR_PARK_MAX_PULSES_DEFAULT   4u
#define MOTOR_ANTI_JAM_REVERSE_MS      1000u
#define MOTOR_ANTI_JAM_WIGGLE_MS        500u
#define MOTOR_ANTI_JAM_MAX_RETRIES        3u
#define MOTOR_CTRL_CMD_QUEUE_DEPTH        4u
#define MOTOR_CTRL_LOOP_SLICE_MS           10u
#define MOTOR_INDEX_IRQ_DEBOUNCE_MS        50u
#define MOTOR_BURST_BEAM_POLL_MS           50u
#define MOTOR_PARK_BEAM_POLL_MS           200u

#endif /* MOTOR_JAM_H */
