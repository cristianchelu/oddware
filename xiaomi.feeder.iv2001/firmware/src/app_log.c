#include "app_log.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifdef HOST_TEST
#include "fake_time.h"
#include "FreeRTOS.h"
#else
#include "FreeRTOS.h"
#include "task.h"

extern int log_write(char *buf, int len);
#endif

#define APP_LOG_LINE_MAX 256

static void (*s_mirror_sink)(const char *buf, size_t len, void *ctx);
static void *s_mirror_ctx;

#ifdef HOST_TEST
static char s_last_line[APP_LOG_LINE_MAX];
#define APP_LOG_TEST_HISTORY  8u
static char s_log_history[APP_LOG_TEST_HISTORY][APP_LOG_LINE_MAX];
static uint8_t s_log_history_depth;
static uint8_t s_log_history_head;
#endif

static uint32_t app_log_now_ms(void)
{
#ifdef HOST_TEST
    return (uint32_t)fake_time_ticks();
#else
    return (uint32_t)(xTaskGetTickCount() * (uint32_t)portTICK_PERIOD_MS);
#endif
}

static void app_log_format_time(char *buf, size_t len, uint32_t ms)
{
    unsigned hours;
    unsigned minutes;
    unsigned seconds;
    unsigned millis;

    hours = ms / 3600000u;
    minutes = (ms % 3600000u) / 60000u;
    seconds = (ms % 60000u) / 1000u;
    millis = ms % 1000u;
    (void)snprintf(buf, len, "%02u:%02u:%02u.%03u", hours, minutes, seconds, millis);
}

static void app_log_write_raw(const char *buf, size_t len)
{
#ifdef HOST_TEST
    size_t copy_len;
    size_t i;

    copy_len = len;
    if (copy_len >= APP_LOG_LINE_MAX) {
        copy_len = APP_LOG_LINE_MAX - 1u;
    }

    for (i = 0; i < copy_len; i++) {
        s_last_line[i] = buf[i];
    }
    while (copy_len > 0u &&
           (s_last_line[copy_len - 1u] == '\r' || s_last_line[copy_len - 1u] == '\n')) {
        copy_len--;
    }
    s_last_line[copy_len] = '\0';

#ifdef HOST_TEST
    {
        uint8_t slot = s_log_history_head;

        (void)memcpy(s_log_history[slot], s_last_line, copy_len + 1u);
        s_log_history_head = (uint8_t)((slot + 1u) % APP_LOG_TEST_HISTORY);
        if (s_log_history_depth < APP_LOG_TEST_HISTORY) {
            s_log_history_depth++;
        }
    }
#endif

    (void)fwrite(buf, 1, len, stdout);
    (void)fflush(stdout);
#else
    if (len > 0u) {
        (void)log_write((char *)buf, (int)len);
    }
#endif

    if (s_mirror_sink != NULL) {
        s_mirror_sink(buf, len, s_mirror_ctx);
    }
}

static void app_log_emit(const char *tag, const char *fmt, va_list ap)
{
    char line[APP_LOG_LINE_MAX];
    char ts[20];
    int prefix_len;
    int body_len;
    size_t total;

    if (tag == NULL || fmt == NULL) {
        return;
    }

    app_log_format_time(ts, sizeof(ts), app_log_now_ms());
    prefix_len = snprintf(line, sizeof(line), "[%s] [%s] ", ts, tag);
    if (prefix_len < 0 || (size_t)prefix_len >= sizeof(line)) {
        return;
    }

    body_len = vsnprintf(line + prefix_len, sizeof(line) - (size_t)prefix_len, fmt, ap);
    if (body_len < 0) {
        return;
    }

    total = (size_t)prefix_len + (size_t)body_len;
    if (total + 2u >= sizeof(line)) {
        total = sizeof(line) - 3u;
    }

    line[total++] = '\r';
    line[total++] = '\n';
    app_log_write_raw(line, total);
}

void app_log_init(void)
{
    s_mirror_sink = NULL;
    s_mirror_ctx = NULL;
}

void app_log_set_sink(void (*fn)(const char *buf, size_t len, void *ctx), void *ctx)
{
    s_mirror_sink = fn;
    s_mirror_ctx = ctx;
}

void app_log_clear_sink(void)
{
    s_mirror_sink = NULL;
    s_mirror_ctx = NULL;
}

void app_log_debug(const char *tag, const char *fmt, ...)
{
#if APP_LOG_LEVEL <= APP_LOG_LEVEL_DEBUG
    va_list ap;

    va_start(ap, fmt);
    app_log_emit(tag, fmt, ap);
    va_end(ap);
#else
    (void)tag;
    (void)fmt;
#endif
}

void app_log_info(const char *tag, const char *fmt, ...)
{
#if APP_LOG_LEVEL <= APP_LOG_LEVEL_INFO
    va_list ap;

    va_start(ap, fmt);
    app_log_emit(tag, fmt, ap);
    va_end(ap);
#else
    (void)tag;
    (void)fmt;
#endif
}

void app_log_warn(const char *tag, const char *fmt, ...)
{
#if APP_LOG_LEVEL <= APP_LOG_LEVEL_WARN
    va_list ap;

    va_start(ap, fmt);
    app_log_emit(tag, fmt, ap);
    va_end(ap);
#else
    (void)tag;
    (void)fmt;
#endif
}

void app_log_error(const char *tag, const char *fmt, ...)
{
#if APP_LOG_LEVEL <= APP_LOG_LEVEL_ERROR
    va_list ap;

    va_start(ap, fmt);
    app_log_emit(tag, fmt, ap);
    va_end(ap);
#else
    (void)tag;
    (void)fmt;
#endif
}

#ifdef HOST_TEST
void app_log_test_reset(void)
{
    s_last_line[0] = '\0';
    s_log_history_depth = 0u;
    s_log_history_head = 0u;
    s_mirror_sink = NULL;
    s_mirror_ctx = NULL;
}

const char *app_log_test_last_line(void)
{
    return s_last_line;
}

const char *app_log_test_line_from_end(unsigned n)
{
    uint8_t idx;

    if (n >= s_log_history_depth) {
        return NULL;
    }

    idx = (uint8_t)((s_log_history_head + APP_LOG_TEST_HISTORY - 1u - n) %
                    APP_LOG_TEST_HISTORY);
    return s_log_history[idx];
}
#endif
