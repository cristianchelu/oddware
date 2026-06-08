/*
 * Captive-portal GET/POST handlers — spec/30-processes/provisioning-flow.md
 */

#include <string.h>

#include "provision_form.h"
#include "provision_pending.h"
#include "provision_portal.h"

static bool provision_portal_query_has_rescan(const char *query, size_t query_len)
{
    const char *needle = "rescan=1";
    size_t needle_len = strlen(needle);

    if (query == NULL || query_len < needle_len) {
        return false;
    }

    for (size_t i = 0; i + needle_len <= query_len; i++) {
        if (memcmp(query + i, needle, needle_len) == 0) {
            return true;
        }
    }

    return false;
}

static size_t provision_portal_render_form(const provision_input_t *input,
                                           const char *message,
                                           provision_scan_list_t *scan,
                                           char *html,
                                           size_t len)
{
    provision_pending_set(input, message);
    return provision_form_render(input, message, scan, html, len);
}

size_t provision_portal_handle_get(const char *query,
                                   size_t query_len,
                                   char *html,
                                   size_t len,
                                   const provision_portal_deps_t *deps)
{
    provision_input_t input;
    char message[PROVISION_PENDING_MSG_MAX];

    if (html == NULL || len == 0 || deps == NULL || deps->scan == NULL) {
        return 0;
    }

    if (deps->active && provision_portal_query_has_rescan(query, query_len)
        && deps->refresh_scan != NULL) {
        deps->refresh_scan();
    }

    if (provision_pending_peek(&input, message, sizeof(message))) {
        return provision_form_render(&input, message, deps->scan, html, len);
    }

    return provision_form_render(NULL, NULL, deps->scan, html, len);
}

size_t provision_portal_handle_post(const char *body,
                                    size_t body_len,
                                    char *html,
                                    size_t len,
                                    const provision_portal_deps_t *deps)
{
    provision_input_t input;
    provision_flow_result_t result;
    size_t html_len;

    if (html == NULL || len == 0 || deps == NULL || deps->scan == NULL
        || deps->flow_submit == NULL) {
        return 0;
    }

    if (body == NULL || body_len == 0) {
        return provision_form_render(NULL, PROVISION_MSG_VALIDATION, deps->scan, html, len);
    }

    if (provision_form_parse_urlencoded(body, body_len, &input) != PORT_OK) {
        return provision_form_render(NULL, PROVISION_MSG_VALIDATION, deps->scan, html, len);
    }

    provision_pending_clear();

    result = deps->flow_submit(&input);

    if (result == PROVISION_FLOW_OK) {
        provision_pending_clear();
        html_len = provision_form_render_success(html, len);
        if (html_len > 0 && deps->on_success != NULL) {
            deps->on_success();
        }
        return html_len;
    }

    if (result == PROVISION_FLOW_WIFI_FAIL) {
        char msg[160];

        if (deps->request_restore != NULL) {
            deps->request_restore(PROVISION_PORTAL_RESTORE_HTTP_ONLY);
        }

        if (provision_form_wifi_fail_message(input.wifi_ssid, msg, sizeof(msg)) == 0) {
            return provision_portal_render_form(&input,
                                                PROVISION_MSG_WIFI_FAIL,
                                                deps->scan,
                                                html,
                                                len);
        }

        return provision_portal_render_form(&input, msg, deps->scan, html, len);
    }

    if (result == PROVISION_FLOW_MQTT_FAIL || result == PROVISION_FLOW_SAVE_FAIL) {
        if (deps->request_restore != NULL) {
            deps->request_restore(PROVISION_PORTAL_RESTORE_AP_PORTAL);
        }
        return provision_portal_render_form(&input,
                                           provision_flow_message(result),
                                           deps->scan,
                                           html,
                                           len);
    }

    return provision_portal_render_form(&input,
                                        provision_flow_message(result),
                                        deps->scan,
                                        html,
                                        len);
}
