/*
 * Wi-Fi STA bring-up — spec/30-processes/wifi-lifecycle.md, uart-console.md
 *
 * Connect timeout comes from wifi_boot_connect_timeout_ms(active bank).
 */

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "app_log.h"
#include "app_event.h"
#include "boot_bank_target.h"
#include "config_port.h"
#include "wifi_cred.h"
#include "wifi_port.h"
#include "wifi_boot_policy.h"
#include "task_def.h"
#ifndef HOST_TEST
#include "wifi_adapter.h"
#endif
#include "wifi_session.h"
#include "wifi_sta.h"

typedef enum {
    WIFI_STA_OP_NONE = 0,
    WIFI_STA_OP_CONNECT,
    WIFI_STA_OP_DISCONNECT,
} wifi_sta_op_t;

static TaskHandle_t s_connect_task;
static volatile bool s_connect_busy;
static volatile bool s_suspended_for_ota;
static volatile bool s_task_reclaimed;
static bool s_unit_test_mode;
static wifi_sta_op_t s_pending_op;

static void wifi_sta_post(app_event_type_t type)
{
    app_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    (void)app_event_post(&ev);
}

static void wifi_sta_run_connect(void)
{
    char ssid[WIFI_SSID_MAX_LEN + 1];
    char pass[WIFI_PASS_MAX_LEN + 1];
    const wifi_port_t *wifi = wifi_port_get();
    char ip[20];
    app_event_t ev;
    TickType_t t0 = xTaskGetTickCount();

    if (wifi_cred_load(config_port_get(),
                       ssid, sizeof(ssid),
                       pass, sizeof(pass)) != PORT_OK) {
        APP_LOG_E("wifi", "no valid credentials in NVDM");
        wifi_sta_post(EVT_WIFI_STA_FAILED);
        return;
    }

    APP_LOG_I("wifi", "connecting to \"%s\" tick=%lu",
              ssid, (unsigned long)t0);

    if (wifi_session_connect(ssid, pass,
                             wifi_boot_connect_timeout_ms(boot_bank_query_active())) != PORT_OK) {
        APP_LOG_E("wifi", "connect failed");
        wifi_sta_post(EVT_WIFI_STA_FAILED);
        return;
    }

    APP_LOG_I("wifi", "session connect done +%lu ms",
              (unsigned long)(xTaskGetTickCount() - t0) * portTICK_PERIOD_MS);

    memset(&ev, 0, sizeof(ev));
    if (wifi->get_ip(ip, sizeof(ip)) == PORT_OK) {
        APP_LOG_I("wifi", "STA ready, IP %s +%lu ms",
                  ip,
                  (unsigned long)(xTaskGetTickCount() - t0) * portTICK_PERIOD_MS);
        ev.type = EVT_WIFI_STA_READY;
        strncpy(ev.u.wifi_ready.ip, ip, sizeof(ev.u.wifi_ready.ip) - 1);
        (void)app_event_post(&ev);
    } else {
        APP_LOG_E("wifi", "connect failed");
        wifi_sta_post(EVT_WIFI_STA_FAILED);
    }
}

static void wifi_sta_run_disconnect(void)
{
    TickType_t t0 = xTaskGetTickCount();

    APP_LOG_I("wifi", "disconnecting tick=%lu", (unsigned long)t0);

    if (wifi_session_down() != PORT_OK) {
        APP_LOG_E("wifi", "disconnect failed");
        return;
    }

    APP_LOG_I("wifi", "STA down +%lu ms",
              (unsigned long)(xTaskGetTickCount() - t0) * portTICK_PERIOD_MS);
}

static void wifi_sta_worker_run(wifi_sta_op_t op)
{
    if (op == WIFI_STA_OP_CONNECT) {
        wifi_sta_run_connect();
    } else if (op == WIFI_STA_OP_DISCONNECT) {
        wifi_sta_run_disconnect();
    }

    s_connect_busy = false;
    s_pending_op = WIFI_STA_OP_NONE;
}

#ifndef HOST_TEST

static void wifi_sta_enter_ota_suspend(void)
{
    s_task_reclaimed = true;
    s_connect_task = NULL;
    s_connect_busy = false;
    s_pending_op = WIFI_STA_OP_NONE;
    vTaskDelete(NULL);
}

static bool wifi_sta_wait_suspended(uint32_t timeout_ms)
{
    TickType_t deadline = xTaskGetTickCount() + pdMS_TO_TICKS(timeout_ms);

    while (xTaskGetTickCount() < deadline) {
        if (s_task_reclaimed) {
            return true;
        }
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    return s_task_reclaimed;
}

static void wifi_sta_connect_task(void *param)
{
    (void)param;

    for (;;) {
        if (s_suspended_for_ota) {
            wifi_sta_enter_ota_suspend();
        }

        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (s_suspended_for_ota) {
            continue;
        }

        wifi_sta_worker_run(s_pending_op);
    }
}

static bool wifi_sta_spawn_connect_task(void)
{
    if (s_connect_task != NULL) {
        return true;
    }

    s_task_reclaimed = false;

    if (xTaskCreate(wifi_sta_connect_task,
                    "wifi_sta",
                    APP_TASK_STACKSIZE / sizeof(portSTACK_TYPE),
                    NULL,
                    APP_TASK_PRIO,
                    &s_connect_task) != pdPASS) {
        APP_LOG_E("wifi", "failed to start connect task");
        return false;
    }

    return true;
}

void wifi_sta_start(void)
{
#ifndef HOST_TEST
    wifi_adapter_stack_init();
#endif

    if (!wifi_sta_spawn_connect_task()) {
        return;
    }

    if (wifi_cred_is_stored(config_port_get())) {
        wifi_sta_request_connect();
    }
}

void wifi_sta_suspend_for_ota(void)
{
    TaskHandle_t task;

    s_suspended_for_ota = true;

    if (wifi_sta_wait_suspended(2000u)) {
        return;
    }

    task = s_connect_task;
    if (task != NULL) {
        APP_LOG_W("wifi", "connect task: force delete for ota");
        s_connect_task = NULL;
        s_task_reclaimed = true;
        s_connect_busy = false;
        s_pending_op = WIFI_STA_OP_NONE;
        vTaskDelete(task);
    }
}

void wifi_sta_resume_after_ota(void)
{
    s_suspended_for_ota = false;
    s_task_reclaimed = false;

    if (s_connect_task == NULL) {
        (void)wifi_sta_spawn_connect_task();
    }
}

#else /* HOST_TEST */

void wifi_sta_suspend_for_ota(void)
{
    s_suspended_for_ota = true;
    s_task_reclaimed = true;
    s_connect_busy = false;
    s_pending_op = WIFI_STA_OP_NONE;
}

void wifi_sta_resume_after_ota(void)
{
    s_suspended_for_ota = false;
    s_task_reclaimed = false;
}

#endif /* HOST_TEST */

static bool wifi_sta_queue_op(wifi_sta_op_t op)
{
    if (s_connect_task == NULL) {
        return false;
    }

    if (s_connect_busy) {
        return false;
    }

    s_connect_busy = true;
    s_pending_op = op;

    if (op == WIFI_STA_OP_CONNECT) {
        wifi_sta_post(EVT_WIFI_STA_CONNECTING);
    }

    if (s_unit_test_mode) {
        return true;
    }

    xTaskNotifyGive(s_connect_task);
    return true;
}

bool wifi_sta_request_connect(void)
{
    return wifi_sta_queue_op(WIFI_STA_OP_CONNECT);
}

bool wifi_sta_request_disconnect(void)
{
    return wifi_sta_queue_op(WIFI_STA_OP_DISCONNECT);
}

wifi_sta_busy_t wifi_sta_busy(void)
{
    if (!s_connect_busy) {
        return WIFI_STA_IDLE;
    }

    if (s_pending_op == WIFI_STA_OP_DISCONNECT) {
        return WIFI_STA_BUSY_DISCONNECT;
    }

    return WIFI_STA_BUSY_CONNECT;
}

/* Host-test entry points — not called from firmware. */
void wifi_sta_test_reset(void)
{
    s_connect_task = NULL;
    s_connect_busy = false;
    s_suspended_for_ota = false;
    s_task_reclaimed = false;
    s_unit_test_mode = false;
    s_pending_op = WIFI_STA_OP_NONE;
}

void wifi_sta_test_bootstrap(void)
{
    wifi_sta_test_reset();
    s_connect_task = (TaskHandle_t)(uintptr_t)1;
    s_unit_test_mode = true;
}

void wifi_sta_test_pump(void)
{
    if (!s_unit_test_mode || !s_connect_busy) {
        return;
    }

    wifi_sta_worker_run(s_pending_op);
}
