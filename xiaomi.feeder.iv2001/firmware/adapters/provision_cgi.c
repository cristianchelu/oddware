/*
 * SDK httpd CGI bridge for captive portal — spec/30-processes/provisioning-flow.md
 */

#include <string.h>

#include "cgi.h"
#include "provision.h"
#include "provision_cgi.h"

#define PROVISION_HTML_MAX 2048

/* httpd runs CGI on a small task stack — keep response buffer off the stack. */
static char s_provision_html[PROVISION_HTML_MAX];

int provision_cgi_handler(struct cgi_para *para)
{
    size_t html_len;

    if (para == NULL || para->func == NULL) {
        return -1;
    }

    if (para->cmd == NULL || para->cmd_len <= 0) {
        html_len = provision_handle_get(s_provision_html, sizeof(s_provision_html));
    } else {
        html_len = provision_handle_post(para->cmd,
                                         (size_t)para->cmd_len,
                                         s_provision_html,
                                         sizeof(s_provision_html));
    }

    if (html_len == 0) {
        return -1;
    }

    para->func(para->sd, s_provision_html, html_len);
    return 0;
}
