/*
 * Stashed captive-portal error after AP drop — spec/30-processes/provisioning-flow.md
 */

#ifndef PROVISION_PENDING_H
#define PROVISION_PENDING_H

#include <stddef.h>
#include <stdbool.h>

#include "provision_form.h"

#define PROVISION_PENDING_MSG_MAX 256

void provision_pending_clear(void);

void provision_pending_set(const provision_input_t *input, const char *message);

bool provision_pending_peek(provision_input_t *input,
                            char *message,
                            size_t message_len);

bool provision_pending_take(provision_input_t *input,
                            char *message,
                            size_t message_len);

#endif /* PROVISION_PENDING_H */
