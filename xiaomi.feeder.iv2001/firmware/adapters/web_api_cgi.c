/*
 * SDK httpd CGI bridge for LAN admin UI — spec/30-processes/web-ui.md
 */

#include <stdio.h>
#include <string.h>

#include "cgi.h"
#include "web_proc.h"
#include "web_api.h"
#include "web_api_cgi.h"
#include "web_ui.h"

#define WEB_API_RESP_MAX   4096

static char s_web_api_path[96];
static char s_web_api_resp[WEB_API_RESP_MAX];

void web_api_cgi_bind_path(const char *path)
{
    if (path == NULL) {
        s_web_api_path[0] = '\0';
        return;
    }

    snprintf(s_web_api_path, sizeof(s_web_api_path), "%s", path);
}

static void web_api_cgi_send(struct cgi_para *para,
                             const char *body,
                             size_t body_len)
{
    if (para == NULL || para->func == NULL || body == NULL) {
        return;
    }

    para->func(para->sd, (char *)body, body_len);
}

int web_api_cgi_handler(struct cgi_para *para)
{
    web_api_route_t route;
    const char *method;
    const char *path;
    int written;

    if (para == NULL) {
        return -1;
    }

    path = s_web_api_path[0] != '\0' ? s_web_api_path : "/";
    method = (para->cmd_len > 0) ? "POST" : "GET";
    route = web_api_classify(method, path);
    if (route == WEB_API_ROUTE_UNKNOWN) {
        written = snprintf(s_web_api_resp,
                           sizeof(s_web_api_resp),
                           "{\"ok\":false,\"error\":\"not_found\"}");
        if (written <= 0) {
            return -1;
        }

        web_api_cgi_send(para, s_web_api_resp, (size_t)written);
        return 0;
    }

    if (strcmp(method, "GET") == 0) {
        written = web_api_handle_get(route, s_web_api_resp, sizeof(s_web_api_resp));
        if (written < 0) {
            return -1;
        }

        web_api_cgi_send(para, s_web_api_resp, (size_t)written);
        return 0;
    }

    written = web_api_handle_post(route,
                                  para->cmd,
                                  para->cmd_len > 0 ? (size_t)para->cmd_len : 0u,
                                  s_web_api_resp,
                                  sizeof(s_web_api_resp));
    if (written < 0) {
        return -1;
    }

    web_api_cgi_send(para, s_web_api_resp, (size_t)written);
    return 0;
}

int web_ui_index_serve(struct connstruct *cn)
{
    const uint8_t *gz;
    size_t gz_len;

    if (cn == NULL) {
        return -1;
    }

    if (!web_ui_is_active()) {
        return 0;
    }

    gz = web_ui_gz_data();
    gz_len = web_ui_gz_len();
    if (gz == NULL || gz_len == 0u) {
        return -1;
    }

    return WEB_write_blk(cn, (char *)gz, 0, gz_len);
}
