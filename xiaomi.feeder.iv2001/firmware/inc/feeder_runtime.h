/*
 * Cross-module runtime snapshot — spec/40-architecture/task-model.md § Runtime snapshot
 *
 * Lock-free readable flags updated by their owning supervisor on the app task.
 * Other modules read without including dispense, motor, etc.
 */

#ifndef FEEDER_RUNTIME_H
#define FEEDER_RUNTIME_H

#include <stdbool.h>

void feeder_runtime_init(void);
void feeder_runtime_set_dispense_active(bool active);
bool feeder_runtime_dispense_active(void);

void feeder_runtime_test_reset(void);

#endif /* FEEDER_RUNTIME_H */
