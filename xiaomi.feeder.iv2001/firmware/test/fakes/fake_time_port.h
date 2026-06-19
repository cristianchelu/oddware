#ifndef FAKE_TIME_PORT_H
#define FAKE_TIME_PORT_H

#include <stdbool.h>
#include <stdint.h>

void fake_time_port_reset(void);
void fake_time_port_set_epoch(int64_t epoch);
void fake_time_port_queue_sync_result(bool ok, int64_t epoch);
bool fake_time_port_init_called(void);
bool fake_time_port_sync_pending(void);

#endif /* FAKE_TIME_PORT_H */
