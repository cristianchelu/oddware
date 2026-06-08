#ifndef PROVISION_H
#define PROVISION_H

#include <stddef.h>
#include <stdbool.h>

void provision_start(void);
bool provision_is_active(void);
bool provision_factory_reset(void);

size_t provision_handle_get(const char *query, size_t query_len, char *html, size_t len);
size_t provision_handle_post(const char *body, size_t body_len, char *html, size_t len);

void provision_after_cgi_response(void);

#endif /* PROVISION_H */
