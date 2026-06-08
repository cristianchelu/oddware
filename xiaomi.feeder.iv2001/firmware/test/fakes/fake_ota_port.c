#include <string.h>

#include "fake_ota_port.h"

static fake_ota_port_state_t s_state;

void fake_ota_port_reset(void)
{
    memset(&s_state, 0, sizeof(s_state));
    s_state.start_result = PORT_OK;
    s_state.status = OTA_STATUS_IDLE;
}

void fake_ota_port_set_start_result(port_err_t result)
{
    s_state.start_result = result;
}

void fake_ota_port_set_status(ota_status_t status)
{
    s_state.status = status;
}

void fake_ota_port_emit_progress(const ota_progress_t *progress)
{
    if (s_state.progress_cb != NULL) {
        s_state.progress_cb(progress, s_state.progress_ctx);
    }
}

const fake_ota_port_state_t *fake_ota_port_state(void)
{
    return &s_state;
}

static port_err_t fake_ota_start(const char *url,
                                 const uint8_t *expected_sha512,
                                 bool has_expected_sha512)
{
    s_state.start_calls++;

    if (url != NULL) {
        strncpy(s_state.last_url, url, sizeof(s_state.last_url) - 1);
        s_state.last_url[sizeof(s_state.last_url) - 1] = '\0';
    }

    if (has_expected_sha512 && expected_sha512 != NULL) {
        memcpy(s_state.last_sha512, expected_sha512, sizeof(s_state.last_sha512));
        s_state.last_has_sha512 = true;
    } else {
        memset(s_state.last_sha512, 0, sizeof(s_state.last_sha512));
        s_state.last_has_sha512 = false;
    }

    if (s_state.start_result == PORT_OK) {
        s_state.status = OTA_STATUS_DOWNLOADING;
    }

    return s_state.start_result;
}

static ota_status_t fake_ota_get_status(void)
{
    return s_state.status;
}

static port_err_t fake_ota_abort(void)
{
    s_state.abort_calls++;
    s_state.status = OTA_STATUS_IDLE;
    return PORT_OK;
}

static void fake_ota_set_progress_cb(ota_progress_cb_t cb, void *ctx)
{
    s_state.progress_cb = cb;
    s_state.progress_ctx = ctx;
}

static const ota_port_t s_fake_ota_port = {
    .start = fake_ota_start,
    .get_status = fake_ota_get_status,
    .abort = fake_ota_abort,
    .set_progress_cb = fake_ota_set_progress_cb,
};

const ota_port_t *fake_ota_port_get(void)
{
    return &s_fake_ota_port;
}
