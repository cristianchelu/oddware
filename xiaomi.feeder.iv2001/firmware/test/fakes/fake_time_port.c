/*
 * Fake time port for host tests — spec/40-architecture/ports.md
 */

#include <string.h>

#include "fake_time_port.h"
#include "time_port.h"

static int64_t s_epoch;
static bool s_sync_pending;
static bool s_sync_ok;
static int64_t s_sync_epoch;
static bool s_init_called;

static port_err_t fake_time_init(void)
{
    s_init_called = true;
    return PORT_OK;
}

static port_err_t fake_time_get_utc_epoch(int64_t *epoch_out)
{
    if (epoch_out == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    *epoch_out = s_epoch;
    return PORT_OK;
}

static port_err_t fake_time_request_sync(void)
{
    if (s_sync_pending) {
        return PORT_ERR_BUSY;
    }

    s_sync_pending = true;
    return PORT_OK;
}

static port_err_t fake_time_poll_sync(bool *done_out, bool *ok_out, int64_t *epoch_out)
{
    if (done_out == NULL || ok_out == NULL || epoch_out == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (!s_sync_pending) {
        *done_out = false;
        return PORT_OK;
    }

    *done_out = true;
    *ok_out = s_sync_ok;
    *epoch_out = s_sync_epoch;
    s_sync_pending = false;
    if (s_sync_ok) {
        s_epoch = s_sync_epoch;
    }
    return PORT_OK;
}

static const time_port_t s_fake_time_port = {
    .init = fake_time_init,
    .get_utc_epoch = fake_time_get_utc_epoch,
    .request_sync = fake_time_request_sync,
    .poll_sync = fake_time_poll_sync,
};

const time_port_t *time_port_get(void)
{
    return &s_fake_time_port;
}

void fake_time_port_reset(void)
{
    s_epoch = 0;
    s_sync_pending = false;
    s_sync_ok = true;
    s_sync_epoch = 0;
    s_init_called = false;
}

void fake_time_port_set_epoch(int64_t epoch)
{
    s_epoch = epoch;
}

void fake_time_port_queue_sync_result(bool ok, int64_t epoch)
{
    s_sync_ok = ok;
    s_sync_epoch = epoch;
}

bool fake_time_port_init_called(void)
{
    return s_init_called;
}

bool fake_time_port_sync_pending(void)
{
    return s_sync_pending;
}
