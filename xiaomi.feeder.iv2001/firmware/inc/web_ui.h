/*
 * LAN web UI lifecycle — spec/30-processes/web-ui.md
 */

#ifndef WEB_UI_H
#define WEB_UI_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef WEB_UI_ENABLE
#define WEB_UI_ENABLE 0
#endif

#ifndef WEB_UI_MIN_HEAP
#define WEB_UI_MIN_HEAP 24576u
#endif

void web_ui_start(void);
void web_ui_stop(void);
void web_ui_suspend_for_ota(void);
void web_ui_resume_after_ota(void);
bool web_ui_is_active(void);

const uint8_t *web_ui_gz_data(void);
size_t web_ui_gz_len(void);

#ifdef HOST_TEST
void web_ui_test_reset(void);
void web_ui_test_set_heap_free(size_t bytes);
#endif

#endif /* WEB_UI_H */
