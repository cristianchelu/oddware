/*
 * NTP sync state machine — spec/30-processes/time-sync.md
 */

#ifndef TIME_SYNC_H
#define TIME_SYNC_H

#include <stdbool.h>
#include <stdint.h>

typedef enum {
    TIME_SYNC_REQUEST_OK = 0,
    TIME_SYNC_REQUEST_BUSY,
    TIME_SYNC_REQUEST_NO_NETWORK,
} time_sync_request_result_t;

void time_sync_init(void);
void time_sync_on_wifi_ready(void);
void time_sync_poll(uint32_t now_ms);
time_sync_request_result_t time_sync_request_now(void);
bool time_sync_is_valid(void);
bool time_sync_get_utc_epoch(int64_t *epoch_out);
void time_sync_test_reset(void);

#endif /* TIME_SYNC_H */
