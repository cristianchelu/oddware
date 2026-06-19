/*
 * MT7682 RTC + SNTP time adapter — spec/30-processes/time-sync.md
 */

#include "FreeRTOS.h"
#include "task.h"

#include "hal_rtc.h"
#include "sntp.h"
#include "epoch_calendar.h"
#include "time_port.h"

#define TIME_RTC_MIN_YEAR     2020
#define TIME_SNTP_TIMEOUT_MS  30000u
#define TIME_SNTP_SERVER      "pool.ntp.org"

static char s_sntp_server[] = TIME_SNTP_SERVER;

static volatile bool s_sntp_started;
static volatile bool s_sntp_done;
static volatile bool s_sntp_ok;
static TickType_t s_sntp_start_tick;

static port_err_t time_adapter_init(void)
{
    if (hal_rtc_init() != HAL_RTC_STATUS_OK) {
        return PORT_ERR_IO;
    }

    return PORT_OK;
}

static void time_adapter_sntp_cb(hal_rtc_time_t time)
{
    (void)time;
    s_sntp_ok = true;
    s_sntp_done = true;
}

static port_err_t time_adapter_get_utc_epoch(int64_t *epoch_out)
{
    hal_rtc_time_t rtc;

    if (epoch_out == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (hal_rtc_get_time(&rtc) != HAL_RTC_STATUS_OK) {
        return PORT_ERR_IO;
    }

    if ((int)rtc.rtc_year + 2000 < TIME_RTC_MIN_YEAR) {
        return PORT_ERR_IO;
    }

    *epoch_out = epoch_calendar_from_ymdhms((int)rtc.rtc_year + 2000,
                                            (int)rtc.rtc_mon,
                                            (int)rtc.rtc_day,
                                            (int)rtc.rtc_hour,
                                            (int)rtc.rtc_min,
                                            (int)rtc.rtc_sec,
                                            0);
    return PORT_OK;
}

static port_err_t time_adapter_request_sync(void)
{
    if (s_sntp_started) {
        return PORT_ERR_BUSY;
    }

    s_sntp_done = false;
    s_sntp_ok = false;
    s_sntp_start_tick = xTaskGetTickCount();
    sntp_set_callback(time_adapter_sntp_cb);
    sntp_setservername(0, s_sntp_server);
    sntp_init();
    s_sntp_started = true;
    return PORT_OK;
}

static port_err_t time_adapter_poll_sync(bool *done_out, bool *ok_out, int64_t *epoch_out)
{
    TickType_t elapsed_ms;

    if (done_out == NULL || ok_out == NULL || epoch_out == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (!s_sntp_started) {
        *done_out = false;
        return PORT_OK;
    }

    if (!s_sntp_done) {
        elapsed_ms = (xTaskGetTickCount() - s_sntp_start_tick) * portTICK_PERIOD_MS;
        if (elapsed_ms >= TIME_SNTP_TIMEOUT_MS) {
            sntp_stop();
            s_sntp_started = false;
            s_sntp_done = true;
            s_sntp_ok = false;
        } else {
            *done_out = false;
            return PORT_OK;
        }
    }

    *done_out = true;
    *ok_out = s_sntp_ok;
    s_sntp_started = false;
    sntp_stop();

    if (s_sntp_ok) {
        return time_adapter_get_utc_epoch(epoch_out);
    }

    *epoch_out = 0;
    return PORT_OK;
}

static const time_port_t s_time_port = {
    .init = time_adapter_init,
    .get_utc_epoch = time_adapter_get_utc_epoch,
    .request_sync = time_adapter_request_sync,
    .poll_sync = time_adapter_poll_sync,
};

const time_port_t *time_port_get(void)
{
    return &s_time_port;
}
