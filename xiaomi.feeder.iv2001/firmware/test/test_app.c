/* Tests: spec/30-processes/app-event-loop.md */

#include <stdlib.h>
#include <string.h>

#include "unity.h"

#include "app.h"
#include "app_event.h"
#include "config_keys.h"
#include "fake_button_port.h"
#include "fake_config_port.h"
#include "fake_display_port.h"
#include "fake_mqtt_port.h"
#include "fake_wifi_port.h"
#include "fake_ota_port.h"
#include "fake_weight_port.h"
#include "mqtt_client.h"
#include "mqtt_client_test.h"
#include "mqtt_cred.h"
#include "ota_client.h"
#include "display_presentation.h"
#include "display_wifi_indicator.h"
#include "display_mqtt_indicator.h"

extern void fake_app_event_q_reset(void);
extern size_t fake_app_event_q_depth(void);

static void seed_broker_config(void)
{
    const config_port_t *cfg = fake_config_port_get();

    fake_config_port_reset();
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_save_host(cfg, "broker.local"));
    TEST_ASSERT_EQUAL(PORT_OK, mqtt_cred_save_port(cfg, 1883));
}

static void post_event(app_event_type_t type)
{
    app_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.type = type;
    TEST_ASSERT_TRUE(app_event_post(&ev));
}

static void post_display_tick(uint32_t now_ms)
{
    app_event_t ev;

    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_DISPLAY_TICK;
    ev.u.display_tick.now_ms = now_ms;
    TEST_ASSERT_TRUE(app_event_post(&ev));
}

static void post_timer_tick(void)
{
    post_event(EVT_TIMER_TICK);
}

static char *test_heap_strdup(const char *s)
{
    size_t len;
    char *copy;

    if (s == NULL) {
        return NULL;
    }

    len = strlen(s);
    copy = malloc(len + 1u);
    if (copy != NULL) {
        memcpy(copy, s, len + 1u);
    }
    return copy;
}

void test_app_display_tick_advances_blink(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    app_test_reset();
    fake_display_port_reset();
    display_presentation_reset();
    display_wifi_indicator_connecting();

    post_display_tick(500u);
    app_step();

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[3] & 0x02u);
}

void test_app_wifi_ready_requests_mqtt_and_shows_icon(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    app_test_reset();
    fake_display_port_reset();
    display_presentation_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_bootstrap();

    post_event(EVT_WIFI_STA_READY);
    app_step();
    post_display_tick(0u);
    app_step();

    TEST_ASSERT_TRUE(mqtt_client_connect_in_progress());

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[3] & 0x02u);
}

void test_app_mqtt_message_ota_starts_download(void)
{
    const fake_ota_port_state_t *ota;
    app_event_t ev;
    const char *topic = "petfeeder/ddeeff/cmd/ota";
    const char *payload = "{\"url\":\"http://10.0.0.5/fw.bin\"}";

    app_test_reset();
    fake_mqtt_port_reset();
    fake_ota_port_reset();
    mqtt_client_test_bootstrap();
    mqtt_client_test_set_device_id("ddeeff");
    fake_mqtt_port_get()->connect(NULL);
    ota_client_set_device_id("ddeeff");
    ota_client_start();

    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_MQTT_MESSAGE;
    ev.u.mqtt_message.topic = test_heap_strdup(topic);
    ev.u.mqtt_message.payload = test_heap_strdup(payload);
    ev.u.mqtt_message.len = strlen(payload);
    TEST_ASSERT_TRUE(app_event_post(&ev));
    app_step();

    ota = fake_ota_port_state();
    TEST_ASSERT_EQUAL_UINT(1, ota->start_calls);
}

void test_app_mqtt_message_dispense_stub(void)
{
    app_event_t ev;
    const char *topic = "petfeeder/ddeeff/cmd/dispense";
    const char *payload = "{\"grams\":10}";

    app_test_reset();
    fake_ota_port_reset();
    mqtt_client_test_bootstrap();
    mqtt_client_test_set_device_id("ddeeff");
    ota_client_set_device_id("ddeeff");
    ota_client_start();

    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_MQTT_MESSAGE;
    ev.u.mqtt_message.topic = test_heap_strdup(topic);
    ev.u.mqtt_message.payload = test_heap_strdup(payload);
    ev.u.mqtt_message.len = strlen(payload);
    TEST_ASSERT_TRUE(app_event_post(&ev));
    app_step();

    TEST_ASSERT_EQUAL_UINT(0, fake_ota_port_state()->start_calls);
}

void test_app_mqtt_session_indicator_transitions(void)
{
    uint8_t grids[TM1637_GRID_COUNT];
    app_event_t ev;

    app_test_reset();
    fake_display_port_reset();
    display_presentation_reset();

    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_MQTT_SESSION;
    ev.u.mqtt_session.phase = MQTT_SESSION_CONNECTING;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    app_step();
    post_display_tick(0u);
    app_step();
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x01u, grids[4]);

    ev.u.mqtt_session.phase = MQTT_SESSION_CONNECTED;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    app_step();
    post_display_tick(0u);
    app_step();
    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x02u, grids[4]);
}

void test_app_timer_tick_weight_boot_fsm(void)
{
    size_t count = 0u;
    const fake_weight_op_t *ops;

    app_test_reset();
    fake_weight_port_reset();
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    fake_display_port_reset();
    display_presentation_reset();

    post_event(EVT_APP_BOOT);
    app_step();

    ops = fake_weight_port_ops(&count);
    TEST_ASSERT_EQUAL_UINT(0, count);

    post_timer_tick();
    app_step();

    ops = fake_weight_port_ops(&count);
    TEST_ASSERT_EQUAL_UINT(1u, count);
    TEST_ASSERT_EQUAL(FAKE_WEIGHT_OP_POWER_ON, ops[0].kind);

    post_timer_tick();
    app_step();

    ops = fake_weight_port_ops(&count);
    TEST_ASSERT_TRUE(count >= 2u);
    TEST_ASSERT_EQUAL(FAKE_WEIGHT_OP_READ_GRAMS, ops[count - 1u].kind);
}

void test_app_calibrated_no_sample_shows_blank_not_dash(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    app_test_reset();
    fake_weight_port_reset();
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    fake_weight_port_set_read_err(PORT_ERR_IO);
    fake_display_port_reset();
    display_presentation_reset();

    post_event(EVT_APP_BOOT);
    app_step();
    post_timer_tick();
    app_step();
    post_timer_tick();
    app_step();
    post_display_tick(0u);
    app_step();

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[2]);
    TEST_ASSERT_EQUAL_HEX8(0x10u, grids[3]);
}

void test_app_idle_loop_samples_every_tick(void)
{
    size_t count = 0u;
    size_t reads = 0u;
    const fake_weight_op_t *ops;
    size_t i;

    app_test_reset();
    fake_weight_port_reset();
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    fake_display_port_reset();
    display_presentation_reset();

    post_event(EVT_APP_BOOT);
    app_step();
    post_timer_tick();
    app_step();
    post_timer_tick();
    app_step();

    for (i = 0u; i < 3u; i++) {
        post_display_tick((i + 1u) * 500u);
        app_step();
    }

    ops = fake_weight_port_ops(&count);
    for (i = 0u; i < count; i++) {
        if (ops[i].kind == FAKE_WEIGHT_OP_READ_GRAMS
            || ops[i].kind == FAKE_WEIGHT_OP_TRY_READ_GRAMS) {
            reads++;
        }
    }

    TEST_ASSERT_EQUAL_UINT(4u, reads);
    TEST_ASSERT_EQUAL(FAKE_WEIGHT_OP_READ_GRAMS, ops[1].kind);
    TEST_ASSERT_EQUAL(FAKE_WEIGHT_OP_TRY_READ_GRAMS, ops[count - 1u].kind);
}

void test_app_read_fail_clears_stale_weight_scene(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    app_test_reset();
    fake_weight_port_reset();
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    fake_weight_port_set_read_grams(42);
    fake_display_port_reset();
    display_presentation_reset();

    post_event(EVT_APP_BOOT);
    app_step();
    post_timer_tick();
    app_step();
    post_timer_tick();
    app_step();

    fake_weight_port_set_read_err(PORT_ERR_IO);
    post_display_tick(500u);
    app_step();

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[0]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[1]);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[2]);
    TEST_ASSERT_EQUAL_HEX8(0x10u, grids[3]);
}

void test_app_read_busy_keeps_stale_weight_scene(void)
{
    uint8_t grids[TM1637_GRID_COUNT];

    app_test_reset();
    fake_weight_port_reset();
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    fake_weight_port_set_read_grams(42);
    fake_display_port_reset();
    display_presentation_reset();

    post_event(EVT_APP_BOOT);
    app_step();
    post_timer_tick();
    app_step();
    post_timer_tick();
    app_step();
    post_display_tick(0u);
    app_step();

    fake_weight_port_set_read_err(PORT_ERR_BUSY);
    post_display_tick(500u);
    app_step();

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x00u, grids[0]);
    TEST_ASSERT_EQUAL_HEX8(0x66u, grids[1]);
    TEST_ASSERT_EQUAL_HEX8(0x5Bu, grids[2]);
    TEST_ASSERT_EQUAL_HEX8(0x10u, grids[3]);
}

void test_app_boot_sets_weight_mode(void)
{
    app_test_reset();
    fake_weight_port_reset();
    fake_display_port_reset();
    display_presentation_reset();

    post_event(EVT_APP_BOOT);
    app_step();

    post_timer_tick();
    app_step();

    post_timer_tick();
    post_timer_tick();
    app_step();
    post_display_tick(0u);
    app_step();

    {
        size_t refresh_ops = 0u;
        const fake_display_op_t *dops = fake_display_port_ops(&refresh_ops);

        TEST_ASSERT_TRUE(refresh_ops >= 1u);
        TEST_ASSERT_EQUAL(FAKE_DISPLAY_OP_SHOW_GRIDS, dops[refresh_ops - 1u].kind);
    }
}

void test_app_wifi_ready_autoconnects_when_broker_stored(void)
{
    const fake_mqtt_port_state_t *mqtt;

    app_test_reset();
    fake_mqtt_port_reset();
    seed_broker_config();
    mqtt_client_test_start();
    fake_wifi_port_reset();
    fake_wifi_port_set_sta_up(true, true);

    post_event(EVT_WIFI_STA_READY);
    app_step();
    mqtt_client_step();

    mqtt = fake_mqtt_port_state();
    TEST_ASSERT_GREATER_OR_EQUAL_UINT(1, mqtt->connect_calls);
}

void test_app_event_coalesces_display_ticks(void)
{
    app_event_t ev;

    fake_app_event_q_reset();
    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_DISPLAY_TICK;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_EQUAL_UINT(1u, fake_app_event_q_depth());
}

void test_app_event_coalesces_timer_ticks(void)
{
    app_event_t ev;

    fake_app_event_q_reset();
    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_TIMER_TICK;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_EQUAL_UINT(1u, fake_app_event_q_depth());
}

void test_app_event_coalesces_button_irq(void)
{
    app_event_t ev;

    fake_app_event_q_reset();
    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_BUTTON_IRQ;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_TRUE(app_event_post(&ev));
    TEST_ASSERT_EQUAL_UINT(1u, fake_app_event_q_depth());
}

void test_app_button_press_logs_on_display_tick(void)
{
    char log[48];
    button_sample_t sample = {
        .power_pressed = true,
        .reset_pressed = false,
        .dispense_pressed = false,
    };

    app_test_reset();
    fake_button_port_reset();
    fake_button_port_set_sample(&sample);
    app_test_clear_btn_log();

    post_display_tick(0u);
    app_step();
    post_display_tick(50u);
    app_step();

    TEST_ASSERT_TRUE(app_test_take_btn_log(log, sizeof(log)));
    TEST_ASSERT_EQUAL_STRING("[btn] power pressed", log);
}

void test_app_weight_updates_during_mqtt_connecting(void)
{
    uint8_t grids[TM1637_GRID_COUNT];
    app_event_t ev;

    app_test_reset();
    fake_weight_port_reset();
    fake_weight_port_set_cal_status(WEIGHT_CAL_SUCCESS);
    fake_weight_port_set_read_grams(55);
    fake_display_port_reset();
    display_presentation_reset();

    post_event(EVT_APP_BOOT);
    app_step();
    post_timer_tick();
    app_step();
    post_timer_tick();
    app_step();

    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_MQTT_SESSION;
    ev.u.mqtt_session.phase = MQTT_SESSION_CONNECTING;
    TEST_ASSERT_TRUE(app_event_post(&ev));
    app_step();

    post_display_tick(500u);
    app_step();

    fake_display_port_last_grids(grids);
    TEST_ASSERT_EQUAL_HEX8(0x6Du, grids[1]);
    TEST_ASSERT_EQUAL_HEX8(0x6Du, grids[2]);
}
