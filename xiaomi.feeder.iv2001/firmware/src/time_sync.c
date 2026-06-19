/*
 * NTP sync state machine — spec/30-processes/time-sync.md
 */

#include "time_sync.h"

#include "app_log.h"
#include "mqtt_config.h"
#include "time_port.h"

#define TIME_SYNC_RESYNC_MS  (6u * 60u * 60u * 1000u)
#define TIME_SYNC_RETRY_MS   (60u * 1000u)

typedef enum {
    TIME_SYNC_STATE_UNKNOWN = 0,
    TIME_SYNC_STATE_SYNCING,
    TIME_SYNC_STATE_SYNCED,
} time_sync_state_t;

static time_sync_state_t s_state;
static bool s_wifi_ready;
static uint32_t s_last_success_ms;
static uint32_t s_next_retry_ms;

static void time_sync_on_success(int64_t epoch, uint32_t now_ms)
{
    (void)epoch;

    s_state = TIME_SYNC_STATE_SYNCED;
    s_last_success_ms = now_ms;
    s_next_retry_ms = 0;
    mqtt_config_publish_snapshot();
}

static void time_sync_log_done(bool ok, int64_t epoch)
{
    if (ok) {
        app_log_info("time", "time sync ok utc=%lu", (unsigned long)epoch);
    } else {
        app_log_warn("time", "time sync failed");
    }
}

static void time_sync_on_finish(bool ok, int64_t epoch, uint32_t now_ms)
{
    if (ok) {
        time_sync_on_success(epoch, now_ms);
    } else if (s_last_success_ms != 0u) {
        s_state = TIME_SYNC_STATE_SYNCED;
    } else {
        s_state = TIME_SYNC_STATE_UNKNOWN;
        s_next_retry_ms = now_ms + TIME_SYNC_RETRY_MS;
    }

    time_sync_log_done(ok, epoch);
}

static bool time_sync_start(void)
{
    const time_port_t *tp = time_port_get();

    if (tp == NULL || tp->request_sync == NULL) {
        return false;
    }

    if (tp->request_sync() != PORT_OK) {
        return false;
    }

    s_state = TIME_SYNC_STATE_SYNCING;
    return true;
}

void time_sync_init(void)
{
    const time_port_t *tp = time_port_get();

    s_state = TIME_SYNC_STATE_UNKNOWN;
    s_wifi_ready = false;
    s_last_success_ms = 0;
    s_next_retry_ms = 0;

    if (tp != NULL && tp->init != NULL) {
        (void)tp->init();
    }
}

void time_sync_on_wifi_ready(void)
{
    s_wifi_ready = true;
    s_next_retry_ms = 0;
    (void)time_sync_start();
}

void time_sync_poll(uint32_t now_ms)
{
    const time_port_t *tp = time_port_get();
    bool done = false;
    bool ok = false;
    int64_t epoch = 0;

    if (!s_wifi_ready || tp == NULL || tp->poll_sync == NULL) {
        return;
    }

    if (s_state == TIME_SYNC_STATE_SYNCING) {
        if (tp->poll_sync(&done, &ok, &epoch) != PORT_OK) {
            return;
        }

        if (!done) {
            return;
        }

        time_sync_on_finish(ok, epoch, now_ms);
        return;
    }

    if (s_state == TIME_SYNC_STATE_SYNCED &&
        s_last_success_ms != 0u &&
        (now_ms - s_last_success_ms) >= TIME_SYNC_RESYNC_MS) {
        (void)time_sync_start();
        return;
    }

    if (s_state == TIME_SYNC_STATE_UNKNOWN &&
        s_next_retry_ms != 0u &&
        now_ms >= s_next_retry_ms) {
        s_next_retry_ms = 0;
        (void)time_sync_start();
    }
}

time_sync_request_result_t time_sync_request_now(void)
{
    if (!s_wifi_ready) {
        return TIME_SYNC_REQUEST_NO_NETWORK;
    }

    if (s_state == TIME_SYNC_STATE_SYNCING) {
        return TIME_SYNC_REQUEST_BUSY;
    }

    if (!time_sync_start()) {
        return TIME_SYNC_REQUEST_BUSY;
    }

    s_next_retry_ms = 0;
    return TIME_SYNC_REQUEST_OK;
}

bool time_sync_is_valid(void)
{
    return s_state == TIME_SYNC_STATE_SYNCED;
}

bool time_sync_get_utc_epoch(int64_t *epoch_out)
{
    const time_port_t *tp = time_port_get();

    if (epoch_out == NULL || tp == NULL || tp->get_utc_epoch == NULL) {
        return false;
    }

    return tp->get_utc_epoch(epoch_out) == PORT_OK;
}

void time_sync_test_reset(void)
{
    s_state = TIME_SYNC_STATE_UNKNOWN;
    s_wifi_ready = false;
    s_last_success_ms = 0;
    s_next_retry_ms = 0;
}
