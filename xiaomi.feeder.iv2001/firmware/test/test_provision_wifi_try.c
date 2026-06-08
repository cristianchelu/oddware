/* Tests: spec/30-processes/provisioning-flow.md (STA test-connect) */

#include <string.h>

#include "unity.h"

#include "fake_time.h"
#include "provision_wifi_try.h"

typedef enum {
    OP_HTTP_STOP = 0,
    OP_HTTP_START,
    OP_AP_STOP,
    OP_AP_START,
    OP_STA_CONNECT,
    OP_STA_ABORT,
    OP_COUNT,
} wifi_try_op_t;

static int s_op_order[16];
static int s_op_count;
static int s_sta_abort_calls;
static int s_ap_start_calls;
static char s_ap_start_ssid[32];
static uint8_t s_ap_start_channel;
static bool s_sta_wait_ok;
static port_err_t s_sta_connect_result;

static void wifi_try_reset(void)
{
    s_op_count = 0;
    s_sta_abort_calls = 0;
    s_ap_start_calls = 0;
    s_ap_start_ssid[0] = '\0';
    s_ap_start_channel = 0;
    s_sta_wait_ok = false;
    s_sta_connect_result = PORT_OK;
    fake_time_reset();
}

static void record_op(wifi_try_op_t op)
{
    if (s_op_count < (int)(sizeof(s_op_order) / sizeof(s_op_order[0]))) {
        s_op_order[s_op_count++] = (int)op;
    }
}

static port_err_t dep_http_stop(void)
{
    record_op(OP_HTTP_STOP);
    return PORT_OK;
}

static port_err_t dep_http_start(uint16_t port)
{
    (void)port;
    record_op(OP_HTTP_START);
    return PORT_OK;
}

static port_err_t dep_ap_stop(void)
{
    record_op(OP_AP_STOP);
    return PORT_OK;
}

static port_err_t dep_ap_start(const char *ssid, uint8_t channel)
{
    record_op(OP_AP_START);
    s_ap_start_calls++;
    if (ssid != NULL) {
        strncpy(s_ap_start_ssid, ssid, sizeof(s_ap_start_ssid) - 1);
        s_ap_start_ssid[sizeof(s_ap_start_ssid) - 1] = '\0';
    }
    s_ap_start_channel = channel;
    return PORT_OK;
}

static port_err_t dep_sta_connect(const char *ssid, const char *pass)
{
    (void)ssid;
    (void)pass;
    record_op(OP_STA_CONNECT);
    return s_sta_connect_result;
}

static bool dep_sta_wait_ready(uint32_t timeout_ms)
{
    (void)timeout_ms;
    return s_sta_wait_ok;
}

static void dep_sta_abort(void)
{
    record_op(OP_STA_ABORT);
    s_sta_abort_calls++;
}

static const provision_wifi_try_deps_t s_deps = {
    .http_stop = dep_http_stop,
    .http_start = dep_http_start,
    .ap_stop = dep_ap_stop,
    .ap_start = dep_ap_start,
    .sta_connect = dep_sta_connect,
    .sta_wait_ready = dep_sta_wait_ready,
    .sta_abort = dep_sta_abort,
};

void test_wifi_try_success_skips_abort_and_restore(void)
{
    wifi_try_reset();
    s_sta_wait_ok = true;

    TEST_ASSERT_TRUE(provision_wifi_try_connect("HomeNet",
                                                "password1",
                                                15000,
                                                "PetFeeder-8722",
                                                6,
                                                &s_deps));
    TEST_ASSERT_EQUAL(0, s_sta_abort_calls);
    TEST_ASSERT_EQUAL(0, s_ap_start_calls);
    TEST_ASSERT_EQUAL(3, s_op_count);
    TEST_ASSERT_EQUAL(OP_HTTP_STOP, s_op_order[0]);
    TEST_ASSERT_EQUAL(OP_AP_STOP, s_op_order[1]);
    TEST_ASSERT_EQUAL(OP_STA_CONNECT, s_op_order[2]);
}

void test_wifi_try_timeout_aborts_sta_and_restores_ap(void)
{
    wifi_try_reset();
    s_sta_wait_ok = false;

    TEST_ASSERT_FALSE(provision_wifi_try_connect("HomeNet",
                                                 "wrong",
                                                 15000,
                                                 "PetFeeder-8722",
                                                 6,
                                                 &s_deps));
    TEST_ASSERT_EQUAL(1, s_sta_abort_calls);
    TEST_ASSERT_EQUAL(1, s_ap_start_calls);
    TEST_ASSERT_EQUAL_STRING("PetFeeder-8722", s_ap_start_ssid);
    TEST_ASSERT_EQUAL(6, s_ap_start_channel);
    TEST_ASSERT_EQUAL(6, s_op_count);
    TEST_ASSERT_EQUAL(OP_HTTP_STOP, s_op_order[0]);
    TEST_ASSERT_EQUAL(OP_AP_STOP, s_op_order[1]);
    TEST_ASSERT_EQUAL(OP_STA_CONNECT, s_op_order[2]);
    TEST_ASSERT_EQUAL(OP_STA_ABORT, s_op_order[3]);
    TEST_ASSERT_EQUAL(OP_AP_START, s_op_order[4]);
    TEST_ASSERT_EQUAL(OP_HTTP_START, s_op_order[5]);
    TEST_ASSERT_EQUAL(PROVISION_WIFI_TRY_AP_SETTLE_MS,
                      (uint32_t)(fake_time_ticks() * portTICK_PERIOD_MS));
}

void test_wifi_try_connect_error_still_restores_ap(void)
{
    wifi_try_reset();
    s_sta_connect_result = PORT_ERR_IO;

    TEST_ASSERT_FALSE(provision_wifi_try_connect("HomeNet",
                                                 "wrong",
                                                 15000,
                                                 "PetFeeder-8722",
                                                 6,
                                                 &s_deps));
    TEST_ASSERT_EQUAL(1, s_sta_abort_calls);
    TEST_ASSERT_EQUAL(1, s_ap_start_calls);
    TEST_ASSERT_EQUAL(6, s_op_count);
    TEST_ASSERT_EQUAL(OP_STA_CONNECT, s_op_order[2]);
    TEST_ASSERT_EQUAL(OP_STA_ABORT, s_op_order[3]);
    TEST_ASSERT_EQUAL(OP_AP_START, s_op_order[4]);
    TEST_ASSERT_EQUAL(OP_HTTP_START, s_op_order[5]);
}

static bool s_ap_stop_fail;

static port_err_t dep_ap_stop_maybe_fail(void)
{
    record_op(OP_AP_STOP);
    return s_ap_stop_fail ? PORT_ERR_IO : PORT_OK;
}

void test_wifi_try_ap_stop_failure_still_attempts_sta(void)
{
    provision_wifi_try_deps_t deps = s_deps;

    wifi_try_reset();
    s_ap_stop_fail = true;
    deps.ap_stop = dep_ap_stop_maybe_fail;
    s_sta_wait_ok = false;

    TEST_ASSERT_FALSE(provision_wifi_try_connect("HomeNet",
                                                 "wrong",
                                                 15000,
                                                 "PetFeeder-8722",
                                                 6,
                                                 &deps));
    TEST_ASSERT_EQUAL(1, s_sta_abort_calls);
    TEST_ASSERT_EQUAL(1, s_ap_start_calls);
}
