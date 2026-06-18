/*
 * OTA MQTT command handler — spec/30-processes/ota-flow.md
 */

#include <stdio.h>
#include <string.h>

#include "app_log.h"
#include "boot_bank_target.h"

#include "mqtt_outbox.h"
#include "mqtt_port.h"
#include "mqtt_route.h"
#include "mqtt_topics.h"
#include "display_ota_indicator.h"
#include "ota_client.h"
#include "ota_port.h"
#include "ota_slot_health.h"
#include "ota_url.h"

static char s_device_id[32];
static char s_status_topic[96];

static char ota_client_active_bank_letter(void)
{
    boot_bank_t active = boot_bank_query_active();

    return (active == BOOT_BANK_B) ? 'B' : 'A';
}

static void ota_client_publish_status(const char *state, uint8_t pct, const char *error)
{
    char payload[128];
    int written;

    if (s_status_topic[0] == '\0') {
        app_log_error("ota", "status publish skipped topic unset");
        return;
    }

    written = snprintf(payload,
                       sizeof(payload),
                       "{\"state\":\"%s\",\"pct\":%u,\"error\":\"%s\",\"bank\":\"%c\"}",
                       state,
                       (unsigned)pct,
                       error != NULL ? error : "",
                       ota_client_active_bank_letter());
    if (written <= 0 || (size_t)written >= sizeof(payload)) {
        return;
    }

    if (!mqtt_outbox_enqueue(s_status_topic, payload, (size_t)written, 1, true)) {
        app_log_error("ota", "status enqueue failed topic=%s", s_status_topic);
        return;
    }

    app_log_debug("ota",
                  "status %s pct=%u err=%s",
                  state,
                  (unsigned)pct,
                  error != NULL ? error : "");
}

static void ota_client_on_progress(const ota_progress_t *progress, void *ctx)
{
    const char *state = "idle";
    const char *error = "";

    (void)ctx;

    if (progress == NULL) {
        return;
    }

    switch (progress->status) {
    case OTA_STATUS_PREPARING:
    case OTA_STATUS_CONNECTING:
    case OTA_STATUS_DOWNLOADING:
        state = "downloading";
        break;
    case OTA_STATUS_VERIFYING:
        state = "downloading";
        break;
    case OTA_STATUS_APPLYING:
        state = "applying";
        break;
    case OTA_STATUS_ERROR:
        state = "error";
        error = progress->error != NULL ? progress->error : "download_failed";
        break;
    default:
        state = "idle";
        break;
    }

    ota_client_publish_status(state, progress->pct, error);
    display_ota_indicator_on_progress(progress);
}

void ota_client_start(void)
{
    ota_port_get()->set_progress_cb(ota_client_on_progress, NULL);
}

void ota_client_set_device_id(const char *device_id)
{
    if (device_id == NULL || device_id[0] == '\0') {
        s_device_id[0] = '\0';
        s_status_topic[0] = '\0';
        return;
    }

    strncpy(s_device_id, device_id, sizeof(s_device_id) - 1);
    s_device_id[sizeof(s_device_id) - 1] = '\0';

    if (mqtt_topic_format(s_status_topic, sizeof(s_status_topic), s_device_id, "ota/status") != PORT_OK) {
        s_status_topic[0] = '\0';
    }

    app_log_debug("ota", "device_id=%s status_topic=%s", s_device_id, s_status_topic);
}

void ota_client_on_mqtt_message(const char *topic, const void *payload, size_t len)
{
    char url[OTA_URL_MAX_LEN + 1];
    uint8_t sha512[FLASH_BANK_SHA512_LEN];
    bool has_sha512 = false;
    char payload_buf[512];
    port_err_t err;

    if (topic == NULL || payload == NULL) {
        app_log_error("ota", "cmd dropped: null topic or payload");
        return;
    }

    if (s_device_id[0] == '\0') {
        app_log_error("ota", "cmd dropped: device_id unset (topic=%s len=%u)", topic, (unsigned)len);
        return;
    }

    if (mqtt_route_classify(topic, s_device_id) != MQTT_ROUTE_CMD_OTA) {
        app_log_debug("ota", "cmd ignored: not ota (device_id=%s)", s_device_id);
        return;
    }

    if (len >= sizeof(payload_buf)) {
        ota_client_publish_status("error", 0, "invalid_url");
        return;
    }

    memcpy(payload_buf, payload, len);
    payload_buf[len] = '\0';

    err = ota_cmd_parse(payload_buf, len, url, sizeof(url), sha512, &has_sha512);
    if (err != PORT_OK) {
        app_log_error("ota", "cmd parse failed");
        ota_client_publish_status("error", 0, "invalid_url");
        return;
    }

    app_log_debug("ota", "parsed url=%s sha512=%s", url, has_sha512 ? "yes" : "no");

    ota_client_publish_status("downloading", 0, "");

    err = ota_port_get()->start(url, sha512, has_sha512);
    if (err == PORT_ERR_BUSY) {
        app_log_error("ota", "start busy");
        ota_client_publish_status("error", 0, "already_in_progress");
        return;
    }
    if (err != PORT_OK) {
        app_log_error("ota", "start failed err=%d", (int)err);
        ota_client_publish_status("error", 0, "invalid_url");
        return;
    }

    app_log_info("ota", "download started");
    display_ota_indicator_start();
}

void ota_client_on_mqtt_connected(void)
{
    ota_client_publish_status("idle", 0, "");
}

uint32_t ota_client_poll_ms(void)
{
    return ota_slot_health_poll_ms();
}
