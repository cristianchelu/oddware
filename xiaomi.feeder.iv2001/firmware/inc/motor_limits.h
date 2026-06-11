/*
 * Motor timing limits — spec/30-processes/dispense-cycle.md,
 * spec/30-processes/uart-console.md § motor commands
 */

#ifndef MOTOR_LIMITS_H
#define MOTOR_LIMITS_H

#include <stdint.h>

#define MOTOR_PH_SETTLE_MS  100u
#define MOTOR_RUN_MS_MAX    20000u

#endif /* MOTOR_LIMITS_H */
