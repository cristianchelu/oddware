/*
 * Captive-portal GET/POST handlers — spec/30-processes/provisioning-flow.md
 */

#ifndef PROVISION_PORTAL_H
#define PROVISION_PORTAL_H

#include <stddef.h>
#include <stdbool.h>

#include "config_port.h"
#include "provision_flow.h"
#include "provision_scan_list.h"

typedef enum provision_portal_restore {
    PROVISION_PORTAL_RESTORE_NONE = 0,
    PROVISION_PORTAL_RESTORE_HTTP_ONLY,
    PROVISION_PORTAL_RESTORE_AP_PORTAL,
} provision_portal_restore_t;

typedef struct provision_portal_deps {
    provision_scan_list_t *scan;
    bool active;
    void (*refresh_scan)(void);
    provision_flow_result_t (*flow_submit)(const provision_input_t *input);
    void (*request_restore)(provision_portal_restore_t kind);
    void (*on_success)(void);
} provision_portal_deps_t;

size_t provision_portal_handle_get(const char *query,
                                   size_t query_len,
                                   char *html,
                                   size_t len,
                                   const provision_portal_deps_t *deps);

size_t provision_portal_handle_post(const char *body,
                                    size_t body_len,
                                    char *html,
                                    size_t len,
                                    const provision_portal_deps_t *deps);

#endif /* PROVISION_PORTAL_H */
