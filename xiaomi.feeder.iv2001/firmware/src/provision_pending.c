/*
 * Stashed captive-portal error after AP drop — spec/30-processes/provisioning-flow.md
 */

#include <string.h>

#include "provision_pending.h"

static bool s_pending;
static provision_input_t s_pending_input;
static char s_pending_message[PROVISION_PENDING_MSG_MAX];

void provision_pending_clear(void)
{
    s_pending = false;
    s_pending_message[0] = '\0';
}

void provision_pending_set(const provision_input_t *input, const char *message)
{
    if (input == NULL || message == NULL) {
        return;
    }

    s_pending_input = *input;
    strncpy(s_pending_message, message, sizeof(s_pending_message) - 1);
    s_pending_message[sizeof(s_pending_message) - 1] = '\0';
    s_pending = true;
}

bool provision_pending_peek(provision_input_t *input,
                            char *message,
                            size_t message_len)
{
    if (!s_pending || input == NULL || message == NULL || message_len == 0) {
        return false;
    }

    *input = s_pending_input;
    strncpy(message, s_pending_message, message_len - 1);
    message[message_len - 1] = '\0';
    return true;
}

bool provision_pending_take(provision_input_t *input,
                            char *message,
                            size_t message_len)
{
    if (!provision_pending_peek(input, message, message_len)) {
        return false;
    }

    provision_pending_clear();
    return true;
}
