/*
 * LAN web API — spec/30-processes/web-ui.md
 */

#include "web_api.h"

#include <stdio.h>
#include <stdarg.h>
#include <string.h>

#include "app_cmd_dispatch.h"
#include "dispense.h"
#include "feed_config.h"
#include "feeder_runtime.h"
#include "mqtt_battery.h"
#include "mqtt_bowl_weight.h"
#include "mqtt_config.h"
#include "mqtt_feed_overfill.h"
#include "mqtt_hopper.h"
#include "mqtt_mains.h"
#include "mqtt_route.h"
#include "mqtt_state.h"
#include "port_err.h"
#include "schedule.h"
#include "time_local.h"
#include "time_sync.h"

static mqtt_route_kind_t web_api_post_route_to_mqtt(web_api_route_t route)
{
    switch (route) {
    case WEB_API_POST_SCHEDULE_SET:
        return MQTT_ROUTE_CMD_SCHEDULE_SET;
    case WEB_API_POST_SCHEDULE_DELETE:
        return MQTT_ROUTE_CMD_SCHEDULE_DELETE;
    case WEB_API_POST_SCHEDULE_TOGGLE:
        return MQTT_ROUTE_CMD_SCHEDULE_TOGGLE;
    case WEB_API_POST_SCHEDULE_SKIP:
        return MQTT_ROUTE_CMD_SCHEDULE_SKIP;
    case WEB_API_POST_SCHEDULE_ENABLE:
        return MQTT_ROUTE_CMD_SCHEDULE_ENABLE;
    case WEB_API_POST_SCHEDULE_TODAY:
        return MQTT_ROUTE_CMD_SCHEDULE_TODAY;
    case WEB_API_POST_DISPENSE:
        return MQTT_ROUTE_CMD_DISPENSE;
    case WEB_API_POST_DISPENSE_CANCEL:
        return MQTT_ROUTE_CMD_DISPENSE_CANCEL;
    case WEB_API_POST_FEED_MODE:
        return MQTT_ROUTE_CMD_FEED_MODE;
    case WEB_API_POST_FEED_OVERFILL:
        return MQTT_ROUTE_CMD_FEED_OVERFILL;
    case WEB_API_POST_CONFIG:
        return MQTT_ROUTE_CMD_CONFIG;
    default:
        return MQTT_ROUTE_UNKNOWN;
    }
}

static const char *web_api_hopper_wire(void)
{
    static char hopper_buf[8];

    if (mqtt_hopper_format_wire(hopper_buf, sizeof(hopper_buf))) {
        return hopper_buf;
    }

    return "normal";
}

static int web_api_write_ok(char *resp, size_t resp_len)
{
    int written;

    written = snprintf(resp, resp_len, "{\"ok\":true}");
    if (written <= 0 || (size_t)written >= resp_len) {
        return -1;
    }

    return written;
}

static int web_api_write_error(char *resp, size_t resp_len, const char *error)
{
    int written;

    if (error == NULL) {
        error = "rejected";
    }

    written = snprintf(resp, resp_len, "{\"ok\":false,\"error\":\"%s\"}", error);
    if (written <= 0 || (size_t)written >= resp_len) {
        return -1;
    }

    return written;
}

static int web_api_append(char *buf, size_t len, int off, const char *fmt, ...)
{
    va_list ap;
    int written;

    if (off < 0 || (size_t)off >= len) {
        return -1;
    }

    va_start(ap, fmt);
    written = vsnprintf(buf + off, len - (size_t)off, fmt, ap);
    va_end(ap);

    if (written <= 0 || (size_t)off + (size_t)written >= len) {
        return -1;
    }

    return off + written;
}

static int web_api_append_telemetry_fields(char *buf, size_t len, int off)
{
    char wire[16];
    bool bowl_error;
    bool known;

    known = mqtt_bowl_weight_format_wire(wire, sizeof(wire));
    if (known) {
        off = web_api_append(buf, len, off, ",\"bowl_weight\":\"%s\"", wire);
        if (off < 0) {
            return -1;
        }
    }

    known = mqtt_state_format_bowl_error(&bowl_error);
    if (known) {
        off = web_api_append(buf,
                             len,
                             off,
                             ",\"bowl_error\":%s",
                             bowl_error ? "true" : "false");
        if (off < 0) {
            return -1;
        }
    }

    known = mqtt_battery_format_wire(wire, sizeof(wire));
    if (known) {
        off = web_api_append(buf, len, off, ",\"battery\":\"%s\"", wire);
        if (off < 0) {
            return -1;
        }
    }

    known = mqtt_mains_format_wire(wire, sizeof(wire));
    if (known) {
        off = web_api_append(buf, len, off, ",\"mains\":\"%s\"", wire);
        if (off < 0) {
            return -1;
        }
    }

    return off;
}

static int web_api_format_status(char *buf, size_t len)
{
    time_local_t now;
    schedule_next_t next;
    bool time_ok;
    bool have_next;
    int off;

    have_next = schedule_compute_next(&next);
    time_ok = time_sync_is_valid() && time_local_now(&now);

    if (time_ok) {
        off = snprintf(buf,
                       len,
                       "{\"time_synced\":true,"
                       "\"local_time\":\"%04u-%02u-%02u %02u:%02u:%02u\","
                       "\"hopper\":\"%s\","
                       "\"dispense_busy\":%s,"
                       "\"schedule_enabled\":%s,"
                       "\"today_enabled\":%s",
                       (unsigned)now.year,
                       (unsigned)now.month,
                       (unsigned)now.day,
                       (unsigned)now.hour,
                       (unsigned)now.min,
                       (unsigned)now.sec,
                       web_api_hopper_wire(),
                       feeder_runtime_dispense_active() ? "true" : "false",
                       schedule_global_enabled() ? "true" : "false",
                       schedule_today_enabled() ? "true" : "false");
    } else {
        off = snprintf(buf,
                       len,
                       "{\"time_synced\":false,"
                       "\"hopper\":\"%s\","
                       "\"dispense_busy\":%s,"
                       "\"schedule_enabled\":%s,"
                       "\"today_enabled\":%s",
                       web_api_hopper_wire(),
                       feeder_runtime_dispense_active() ? "true" : "false",
                       schedule_global_enabled() ? "true" : "false",
                       schedule_today_enabled() ? "true" : "false");
    }

    if (off <= 0 || (size_t)off >= len) {
        return -1;
    }

    off = web_api_append_telemetry_fields(buf, len, off);
    if (off < 0) {
        return -1;
    }

    if (time_ok && have_next) {
        off = web_api_append(buf,
                             len,
                             off,
                             ",\"next\":{\"hour\":%u,\"min\":%u,\"g\":%u,\"in_min\":%ld}",
                             (unsigned)next.hour,
                             (unsigned)next.min,
                             (unsigned)next.g,
                             (long)next.in_min);
        if (off < 0) {
            return -1;
        }
    }

    off = web_api_append(buf, len, off, "}");
    return off < 0 ? -1 : off;
}

web_api_route_t web_api_classify(const char *method, const char *path)
{
    bool is_get;
    bool is_post;

    if (method == NULL || path == NULL) {
        return WEB_API_ROUTE_UNKNOWN;
    }

    is_get = strcmp(method, "GET") == 0;
    is_post = strcmp(method, "POST") == 0;

    if (!is_get && !is_post) {
        return WEB_API_ROUTE_UNKNOWN;
    }

    if (is_get) {
        if (strcmp(path, "/api/status") == 0) {
            return WEB_API_GET_STATUS;
        }
        if (strcmp(path, "/api/schedule/state") == 0) {
            return WEB_API_GET_SCHEDULE_STATE;
        }
        if (strcmp(path, "/api/schedule/next") == 0) {
            return WEB_API_GET_SCHEDULE_NEXT;
        }
        if (strcmp(path, "/api/config") == 0) {
            return WEB_API_GET_CONFIG;
        }
        if (strcmp(path, "/api/feed/mode") == 0) {
            return WEB_API_GET_FEED_MODE;
        }
        if (strcmp(path, "/api/feed/overfill") == 0) {
            return WEB_API_GET_FEED_OVERFILL;
        }

        return WEB_API_ROUTE_UNKNOWN;
    }

    if (strcmp(path, "/api/schedule/set") == 0) {
        return WEB_API_POST_SCHEDULE_SET;
    }
    if (strcmp(path, "/api/schedule/delete") == 0) {
        return WEB_API_POST_SCHEDULE_DELETE;
    }
    if (strcmp(path, "/api/schedule/toggle") == 0) {
        return WEB_API_POST_SCHEDULE_TOGGLE;
    }
    if (strcmp(path, "/api/schedule/skip") == 0) {
        return WEB_API_POST_SCHEDULE_SKIP;
    }
    if (strcmp(path, "/api/schedule/enable") == 0) {
        return WEB_API_POST_SCHEDULE_ENABLE;
    }
    if (strcmp(path, "/api/schedule/today") == 0) {
        return WEB_API_POST_SCHEDULE_TODAY;
    }
    if (strcmp(path, "/api/dispense") == 0) {
        return WEB_API_POST_DISPENSE;
    }
    if (strcmp(path, "/api/dispense/cancel") == 0) {
        return WEB_API_POST_DISPENSE_CANCEL;
    }
    if (strcmp(path, "/api/feed/mode") == 0) {
        return WEB_API_POST_FEED_MODE;
    }
    if (strcmp(path, "/api/feed/overfill") == 0) {
        return WEB_API_POST_FEED_OVERFILL;
    }
    if (strcmp(path, "/api/config") == 0) {
        return WEB_API_POST_CONFIG;
    }

    return WEB_API_ROUTE_UNKNOWN;
}

int web_api_handle_get(web_api_route_t route, char *buf, size_t len)
{
    int written;

    if (buf == NULL || len == 0) {
        return -1;
    }

    switch (route) {
    case WEB_API_GET_SCHEDULE_STATE:
        return schedule_format_state_json(buf, len);

    case WEB_API_GET_SCHEDULE_NEXT:
        written = schedule_format_next_json(buf, len);
        if (written > 0) {
            return written;
        }

        if (len < 3) {
            return -1;
        }

        buf[0] = '{';
        buf[1] = '}';
        buf[2] = '\0';
        return 2;

    case WEB_API_GET_CONFIG:
        if (!mqtt_config_format_snapshot(buf, len)) {
            return -1;
        }

        return (int)strlen(buf);

    case WEB_API_GET_FEED_MODE:
        written = snprintf(buf,
                           len,
                           "%s",
                           feed_config_mode_string(feed_config_mode_get()));
        if (written <= 0 || (size_t)written >= len) {
            return -1;
        }

        return written;

    case WEB_API_GET_FEED_OVERFILL:
        if (!mqtt_feed_overfill_format_snapshot(buf, len)) {
            return -1;
        }

        return (int)strlen(buf);

    case WEB_API_GET_STATUS:
        return web_api_format_status(buf, len);

    default:
        return -1;
    }
}

int web_api_handle_post(web_api_route_t route,
                        const char *body,
                        size_t body_len,
                        char *resp,
                        size_t resp_len)
{
    mqtt_route_kind_t mqtt_route;
    port_err_t err;

    if (resp == NULL || resp_len == 0) {
        return -1;
    }

    mqtt_route = web_api_post_route_to_mqtt(route);
    if (mqtt_route == MQTT_ROUTE_UNKNOWN) {
        return web_api_write_error(resp, resp_len, "unknown_route");
    }

    err = app_cmd_dispatch(mqtt_route, body, body_len, "web");
    if (err == PORT_OK) {
        return web_api_write_ok(resp, resp_len);
    }

    if (err == PORT_ERR_BUSY) {
        return web_api_write_error(resp, resp_len, "busy");
    }

    if (err == PORT_ERR_NOT_SUPPORTED) {
        return web_api_write_error(resp, resp_len, "not_supported");
    }

    return web_api_write_error(resp, resp_len, "rejected");
}
