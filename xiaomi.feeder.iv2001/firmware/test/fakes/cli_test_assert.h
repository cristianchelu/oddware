#ifndef CLI_TEST_ASSERT_H
#define CLI_TEST_ASSERT_H

#include <stdio.h>
#include <string.h>

#include "unity.h"
#include "app_log.h"

static inline void cli_test_reset(void)
{
    app_log_test_reset();
}

static inline void assert_cli_body(const char *body)
{
    const char *line = app_log_test_last_line();

    TEST_ASSERT_NOT_NULL(strstr(line, "[cli]"));
    TEST_ASSERT_NOT_NULL(strstr(line, body));
}

static inline void assert_log_body(const char *tag, const char *body)
{
    char bracket_tag[32];
    const char *line = app_log_test_last_line();

    (void)snprintf(bracket_tag, sizeof(bracket_tag), "[%s]", tag);
    TEST_ASSERT_NOT_NULL(strstr(line, bracket_tag));
    TEST_ASSERT_NOT_NULL(strstr(line, body));
}

#endif /* CLI_TEST_ASSERT_H */
