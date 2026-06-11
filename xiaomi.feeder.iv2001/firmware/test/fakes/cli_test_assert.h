#ifndef CLI_TEST_ASSERT_H
#define CLI_TEST_ASSERT_H

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

#endif /* CLI_TEST_ASSERT_H */
