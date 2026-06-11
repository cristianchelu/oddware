/*
 * Host-test helpers for motor_ctrl — not used in production firmware.
 */

#ifndef MOTOR_CTRL_TEST_H
#define MOTOR_CTRL_TEST_H

#include <stdint.h>

void motor_ctrl_test_reset(void);
void motor_ctrl_test_poll(void);
void motor_ctrl_test_notify(uint32_t bits);

#endif /* MOTOR_CTRL_TEST_H */
