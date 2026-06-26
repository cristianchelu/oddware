#ifndef APP_LOG_H
#define APP_LOG_H

#include <stddef.h>

#define APP_LOG_LEVEL_DEBUG 0
#define APP_LOG_LEVEL_INFO  1
#define APP_LOG_LEVEL_WARN  2
#define APP_LOG_LEVEL_ERROR 3

#ifndef APP_LOG_LEVEL
#define APP_LOG_LEVEL APP_LOG_LEVEL_DEBUG
#endif

void app_log_init(void);
void app_log_debug(const char *tag, const char *fmt, ...);
void app_log_info(const char *tag, const char *fmt, ...);
void app_log_warn(const char *tag, const char *fmt, ...);
void app_log_error(const char *tag, const char *fmt, ...);
void app_log_set_sink(void (*fn)(const char *buf, size_t len, void *ctx), void *ctx);
void app_log_clear_sink(void);

#define APP_LOG_D(tag, fmt, ...) app_log_debug(tag, fmt, ##__VA_ARGS__)
#define APP_LOG_I(tag, fmt, ...) app_log_info(tag, fmt, ##__VA_ARGS__)
#define APP_LOG_W(tag, fmt, ...) app_log_warn(tag, fmt, ##__VA_ARGS__)
#define APP_LOG_E(tag, fmt, ...) app_log_error(tag, fmt, ##__VA_ARGS__)

#ifdef HOST_TEST
void app_log_test_reset(void);
const char *app_log_test_last_line(void);
const char *app_log_test_line_from_end(unsigned n);
#endif

#endif /* APP_LOG_H */
