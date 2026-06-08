/*
 * OTA port adapter — HTTP download to inactive bank.
 *
 * Downloads firmware using HTTP Range requests to avoid overwhelming the
 * MT7687 connsys RX buffer pool.  Each range is a separate TCP connection
 * carrying at most OTA_RANGE_SIZE bytes of payload.
 */

#include <stdlib.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"
#include "syslog.h"

#include "task_def.h"

#include "httpclient.h"
#include "mbedtls/sha512.h"

#include "flash_bank_logic.h"
#include "flash_bank_port.h"
#include "hal_cache.h"
#include "hal_sys.h"
#include "ota_image.h"
#include "ota_port.h"
#include "ota_rollback.h"
#include "mqtt_client.h"

log_create_module(ota_adapter, PRINT_LEVEL_INFO);

#define OTA_DL_TASK_STACK   (12288)
#define OTA_DL_TASK_PRIO    (TASK_PRIORITY_ABOVE_NORMAL - 1)
#define OTA_RANGE_HDR_BUF   256

typedef struct {
    char url[OTA_URL_MAX_LEN + 1];
    uint8_t expected_sha512[FLASH_BANK_SHA512_LEN];
    bool has_expected_sha512;
} ota_job_t;

static TaskHandle_t s_ota_task;
static volatile ota_status_t s_status = OTA_STATUS_IDLE;
static volatile bool s_abort_requested;
static ota_progress_cb_t s_progress_cb;
static void *s_progress_ctx;
static uint8_t s_image_hash[FLASH_BANK_SHA512_LEN];

static void ota_adapter_report(ota_status_t status, uint8_t pct, const char *error)
{
    ota_progress_t progress;

    s_status = status;
    progress.status = status;
    progress.pct = pct;
    progress.error = error;

    if (s_progress_cb != NULL) {
        s_progress_cb(&progress, s_progress_ctx);
    }
}

static void ota_adapter_reboot(void)
{
    vTaskDelay(pdMS_TO_TICKS(200));
    hal_cache_disable();
    hal_cache_deinit();
    hal_sys_reboot(HAL_SYS_REBOOT_MAGIC, WHOLE_SYSTEM_REBOOT_COMMAND);
}

/*
 * Parse total file size from a Content-Range header value.
 * Expected format: "bytes 0-4095/396652" → returns 396652.
 */
static uint32_t ota_adapter_parse_content_range_total(const char *hdr_buf)
{
    int val_pos = 0;
    int val_len = 0;
    const char *p;
    const char *end;

    if (hdr_buf == NULL) {
        return 0;
    }

    if (httpclient_get_response_header_value((char *)hdr_buf,
                                            "Content-Range",
                                            &val_pos, &val_len) != 0) {
        return 0;
    }

    p = hdr_buf + val_pos;
    end = p + val_len;

    while (p < end && *p != '/') {
        p++;
    }
    if (p >= end) {
        return 0;
    }

    return strtoul(p + 1, NULL, 10);
}

/*
 * Download a single Range chunk.  Opens a fresh TCP connection, sends
 * GET with Range header, receives body into flash, then closes.
 * Returns the number of body bytes written to flash via *range_bytes_out.
 */
static port_err_t ota_adapter_fetch_range(const char *url,
                                          char *chunk_buf,
                                          char *hdr_buf,
                                          int hdr_buf_len,
                                          uint32_t offset,
                                          uint32_t range_end,
                                          uint32_t *file_total_out,
                                          uint32_t *range_bytes_out,
                                          mbedtls_sha512_context *sha_ctx)
{
    httpclient_t client = {0};
    httpclient_data_t client_data = {0};
    char range_hdr[64];
    int32_t ret;
    int32_t recv_temp = -1;
    uint32_t range_downloaded = 0;
    const flash_bank_port_t *flash = flash_bank_port_get();

    snprintf(range_hdr, sizeof(range_hdr),
             "Range: bytes=%lu-%lu\r\n",
             (unsigned long)offset, (unsigned long)range_end);

    client_data.response_buf = chunk_buf;
    client_data.response_buf_len = OTA_CHUNK_SIZE;

    if (hdr_buf != NULL) {
        client_data.header_buf = hdr_buf;
        client_data.header_buf_len = hdr_buf_len;
    }

    httpclient_set_custom_header(&client, range_hdr);

    ret = httpclient_connect(&client, (char *)url);
    if (ret != HTTPCLIENT_OK) {
        printf("[ota] connect fail at %lu\r\n", (unsigned long)offset);
        httpclient_close(&client);
        return PORT_ERR_IO;
    }

    httpclient_set_recv_timeout(&client, 20000);

    ret = httpclient_send_request(&client, (char *)url, HTTPCLIENT_GET, &client_data);
    if (ret != HTTPCLIENT_OK) {
        printf("[ota] send fail at %lu\r\n", (unsigned long)offset);
        httpclient_close(&client);
        return PORT_ERR_IO;
    }

    do {
        uint32_t data_len;

        if (s_abort_requested) {
            httpclient_close(&client);
            return PORT_ERR_BUSY;
        }

        ret = httpclient_recv_response(&client, &client_data);
        if (ret < HTTPCLIENT_OK) {
            printf("[ota] recv fail at %lu+%lu ret=%ld\r\n",
                   (unsigned long)offset, (unsigned long)range_downloaded,
                   (long)ret);
            httpclient_close(&client);
            return PORT_ERR_IO;
        }

        if (recv_temp < 0) {
            recv_temp = client_data.response_content_len;
        }

        data_len = (recv_temp >= 0)
                   ? (uint32_t)(recv_temp - client_data.retrieve_len) : 0;
        recv_temp = client_data.retrieve_len;

        if (data_len > 0) {
            uint32_t write_offset = offset + range_downloaded;

            if (write_offset + data_len > CM4_LENGTH) {
                httpclient_close(&client);
                return PORT_ERR_INVALID_ARG;
            }

            if (flash->write_inactive(write_offset, (const uint8_t *)chunk_buf,
                                      data_len) != PORT_OK) {
                LOG_E(ota_adapter, "flash write fail at %lu len=%lu",
                      (unsigned long)write_offset, (unsigned long)data_len);
                httpclient_close(&client);
                return PORT_ERR_IO;
            }

            mbedtls_sha512_update(sha_ctx, (const unsigned char *)chunk_buf, data_len);
            range_downloaded += data_len;
        }
    } while (ret == HTTPCLIENT_RETRIEVE_MORE_DATA);

    /* Extract total file size from Content-Range on the first range. */
    if (file_total_out != NULL && *file_total_out == 0) {
        int resp_code = httpclient_get_response_code(&client);

        if (resp_code == 206) {
            *file_total_out = ota_adapter_parse_content_range_total(hdr_buf);
        } else {
            LOG_E(ota_adapter, "server returned %d, Range not supported", resp_code);
            httpclient_close(&client);
            return PORT_ERR_IO;
        }

        if (*file_total_out == 0) {
            LOG_E(ota_adapter, "Content-Range total missing");
            httpclient_close(&client);
            return PORT_ERR_IO;
        }

        LOG_I(ota_adapter, "file size %lu bytes", (unsigned long)*file_total_out);
    }

    {
        uint32_t range_expected = range_end - offset + 1;

        if (range_downloaded < range_expected) {
            printf("[ota] range short at %lu got %lu want %lu\r\n",
                   (unsigned long)offset, (unsigned long)range_downloaded,
                   (unsigned long)range_expected);
            httpclient_close(&client);
            return PORT_ERR_IO;
        }
    }

    httpclient_close(&client);
    *range_bytes_out = range_downloaded;
    return PORT_OK;
}

static port_err_t ota_adapter_http_download(const char *url,
                                            uint32_t *downloaded_out,
                                            uint8_t hash_out[FLASH_BANK_SHA512_LEN])
{
    char *chunk_buf = NULL;
    char *hdr_buf = NULL;
    uint32_t downloaded = 0;
    uint32_t total = 0;
    uint8_t last_report_pct = 0;
    mbedtls_sha512_context ctx;
    port_err_t err;

    chunk_buf = pvPortMalloc(OTA_CHUNK_SIZE);
    if (chunk_buf == NULL) {
        return PORT_ERR_IO;
    }

    hdr_buf = pvPortMalloc(OTA_RANGE_HDR_BUF);
    if (hdr_buf == NULL) {
        vPortFree(chunk_buf);
        return PORT_ERR_IO;
    }

    mbedtls_sha512_init(&ctx);
    mbedtls_sha512_starts(&ctx, 0);

    flash_bank_port_get()->erase_inactive();

    printf("[ota] range download start heap=%u min=%u\r\n",
           (unsigned)xPortGetFreeHeapSize(),
           (unsigned)xPortGetMinimumEverFreeHeapSize());

    while (!s_abort_requested) {
        uint32_t range_end;
        uint32_t range_bytes = 0;
        int retries;

        range_end = downloaded + OTA_RANGE_SIZE - 1;
        if (total > 0 && range_end >= total) {
            range_end = total - 1;
        }

        err = PORT_ERR_IO;
        for (retries = 0; retries < 3; retries++) {
            if (retries > 0) {
                printf("[ota] retry %d at %lu\r\n", retries, (unsigned long)downloaded);
                vTaskDelay(pdMS_TO_TICKS(1000));
            }

            err = ota_adapter_fetch_range(
                    url, chunk_buf,
                    (total == 0) ? hdr_buf : NULL,
                    OTA_RANGE_HDR_BUF,
                    downloaded, range_end,
                    &total, &range_bytes, &ctx);

            if (err == PORT_OK) {
                break;
            }
        }

        if (err != PORT_OK) {
            printf("[ota] range fail at %lu after retries\r\n", (unsigned long)downloaded);
            mbedtls_sha512_free(&ctx);
            if (hdr_buf != NULL) {
                vPortFree(hdr_buf);
            }
            vPortFree(chunk_buf);
            return err;
        }

        /* Free header buffer once total is known. */
        if (hdr_buf != NULL && total > 0) {
            vPortFree(hdr_buf);
            hdr_buf = NULL;
        }

        downloaded += range_bytes;

        if (total > 0) {
            uint8_t pct = ota_progress_pct(downloaded, total);
            if (pct >= last_report_pct + OTA_PROGRESS_STEP_PCT || pct == 100) {
                printf("[ota] %u%% (%lu/%lu) heap=%u min=%u\r\n",
                   pct, (unsigned long)downloaded, (unsigned long)total,
                   (unsigned)xPortGetFreeHeapSize(),
                   (unsigned)xPortGetMinimumEverFreeHeapSize());
                last_report_pct = pct;
                ota_adapter_report(OTA_STATUS_DOWNLOADING, pct, "");
            }
        }

        if (total > 0 && downloaded >= total) {
            break;
        }

        if (range_bytes == 0) {
            break;
        }

        vTaskDelay(pdMS_TO_TICKS(OTA_RANGE_DELAY_MS));
    }

    if (hdr_buf != NULL) {
        vPortFree(hdr_buf);
    }

    if (s_abort_requested) {
        mbedtls_sha512_free(&ctx);
        vPortFree(chunk_buf);
        return PORT_ERR_BUSY;
    }

    if (downloaded == 0 || (total > 0 && downloaded != total)) {
        LOG_E(ota_adapter, "download incomplete bytes=%lu total=%lu",
              (unsigned long)downloaded, (unsigned long)total);
        mbedtls_sha512_free(&ctx);
        vPortFree(chunk_buf);
        return PORT_ERR_IO;
    }

    mbedtls_sha512_finish(&ctx, hash_out);
    mbedtls_sha512_free(&ctx);
    vPortFree(chunk_buf);

    if (!ota_image_size_allowed(downloaded)) {
        return PORT_ERR_INVALID_ARG;
    }

    {
        boot_bank_t inactive = flash_bank_inactive(flash_bank_port_get()->get_active_bank());
        uint32_t bank_base = flash_bank_rom_offset(inactive);

        if (ota_image_check_vector_table_in_bank(bank_base) != PORT_OK) {
            printf("[ota] vector table not found in bank\r\n");
            return PORT_ERR_INVALID_ARG;
        }
    }

    *downloaded_out = downloaded;
    printf("[ota] download complete bytes=%lu\r\n", (unsigned long)downloaded);
    return PORT_OK;
}

static void ota_adapter_task_fail(const char *error)
{
    mqtt_client_resume_after_ota();
    ota_adapter_report(OTA_STATUS_ERROR, 0, error);
    s_status = OTA_STATUS_IDLE;
    s_ota_task = NULL;
    vTaskDelete(NULL);
}

static void ota_adapter_task(void *param)
{
    ota_job_t job;
    const flash_bank_port_t *flash = flash_bank_port_get();
    uint32_t downloaded = 0;
    port_err_t err;

    (void)param;

    memcpy(&job, param, sizeof(job));
    vPortFree(param);

    s_abort_requested = false;
    LOG_I(ota_adapter, "task start url=%s sha512=%s", job.url, job.has_expected_sha512 ? "yes" : "no");
    s_status = OTA_STATUS_DOWNLOADING;

    mqtt_client_suspend_for_ota();
    if (!mqtt_client_wait_disconnected(5000)) {
        LOG_E(ota_adapter, "mqtt disconnect timeout");
        printf("[ota] mqtt disconnect timeout\r\n");
    }
    vTaskDelay(pdMS_TO_TICKS(3000));
    printf("[ota] mqtt down, http start\r\n");

    err = ota_adapter_http_download(job.url, &downloaded, s_image_hash);
    if (err != PORT_OK) {
        const char *error = (err == PORT_ERR_INVALID_ARG) ? "image_too_large" : "download_failed";
        LOG_E(ota_adapter, "download error=%s err=%d", error, (int)err);
        ota_adapter_task_fail(error);
        return;
    }

    ota_adapter_report(OTA_STATUS_VERIFYING, 100, "");

    if (job.has_expected_sha512 &&
        memcmp(s_image_hash, job.expected_sha512, FLASH_BANK_SHA512_LEN) != 0) {
        LOG_E(ota_adapter, "sha512 mismatch");
        ota_adapter_task_fail("verify_failed");
        return;
    }

    if (flash->verify_inactive(s_image_hash, downloaded) != PORT_OK) {
        LOG_E(ota_adapter, "flash verify failed");
        ota_adapter_task_fail("verify_failed");
        return;
    }

    ota_adapter_report(OTA_STATUS_APPLYING, 100, "");
    LOG_I(ota_adapter, "bank swap pending");

    ota_rollback_mark_pending();

    if (flash->swap_banks(s_image_hash) != PORT_OK) {
        LOG_E(ota_adapter, "bank swap failed");
        ota_adapter_task_fail("download_failed");
        return;
    }

    ota_adapter_reboot();
}

static port_err_t ota_adapter_start(const char *url,
                                    const uint8_t *expected_sha512,
                                    bool has_expected_sha512)
{
    ota_job_t *job;

    if (url == NULL || url[0] == '\0') {
        return PORT_ERR_INVALID_ARG;
    }

    if (s_ota_task != NULL || s_status != OTA_STATUS_IDLE) {
        LOG_E(ota_adapter, "start busy status=%d", (int)s_status);
        return PORT_ERR_BUSY;
    }

    job = pvPortMalloc(sizeof(*job));
    if (job == NULL) {
        return PORT_ERR_IO;
    }

    strncpy(job->url, url, sizeof(job->url) - 1);
    job->url[sizeof(job->url) - 1] = '\0';
    job->has_expected_sha512 = has_expected_sha512;
    if (has_expected_sha512 && expected_sha512 != NULL) {
        memcpy(job->expected_sha512, expected_sha512, FLASH_BANK_SHA512_LEN);
    }

    s_status = OTA_STATUS_DOWNLOADING;

    if (xTaskCreate(ota_adapter_task,
                    "ota_dl",
                    OTA_DL_TASK_STACK / sizeof(portSTACK_TYPE),
                    job,
                    OTA_DL_TASK_PRIO,
                    &s_ota_task) != pdPASS) {
        LOG_E(ota_adapter, "task create failed");
        vPortFree(job);
        s_status = OTA_STATUS_IDLE;
        return PORT_ERR_IO;
    }

    LOG_I(ota_adapter, "task created url=%s", url);
    return PORT_OK;
}

static ota_status_t ota_adapter_get_status(void)
{
    return s_status;
}

static port_err_t ota_adapter_abort(void)
{
    if (s_ota_task == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    s_abort_requested = true;
    return PORT_OK;
}

static void ota_adapter_set_progress_cb(ota_progress_cb_t cb, void *ctx)
{
    s_progress_cb = cb;
    s_progress_ctx = ctx;
}

static const ota_port_t s_ota_port = {
    .start = ota_adapter_start,
    .get_status = ota_adapter_get_status,
    .abort = ota_adapter_abort,
    .set_progress_cb = ota_adapter_set_progress_cb,
};

const ota_port_t *ota_port_get(void)
{
    return &s_ota_port;
}
