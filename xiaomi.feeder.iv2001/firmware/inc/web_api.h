/*
 * LAN web API — spec/30-processes/web-ui.md
 */

#ifndef WEB_API_H
#define WEB_API_H

#include <stddef.h>
#include <stdint.h>

typedef enum {
    WEB_API_ROUTE_UNKNOWN = 0,
    WEB_API_GET_STATUS,
    WEB_API_GET_SCHEDULE_STATE,
    WEB_API_GET_SCHEDULE_NEXT,
    WEB_API_GET_CONFIG,
    WEB_API_GET_FEED_MODE,
    WEB_API_GET_FEED_OVERFILL,
    WEB_API_POST_SCHEDULE_SET,
    WEB_API_POST_SCHEDULE_DELETE,
    WEB_API_POST_SCHEDULE_TOGGLE,
    WEB_API_POST_SCHEDULE_SKIP,
    WEB_API_POST_SCHEDULE_ENABLE,
    WEB_API_POST_SCHEDULE_TODAY,
    WEB_API_POST_DISPENSE,
    WEB_API_POST_DISPENSE_CANCEL,
    WEB_API_POST_FEED_MODE,
    WEB_API_POST_FEED_OVERFILL,
    WEB_API_POST_CONFIG,
} web_api_route_t;

web_api_route_t web_api_classify(const char *method, const char *path);
int web_api_handle_get(web_api_route_t route, char *buf, size_t len);
int web_api_handle_post(web_api_route_t route,
                        const char *body,
                        size_t body_len,
                        char *resp,
                        size_t resp_len);

#endif /* WEB_API_H */
