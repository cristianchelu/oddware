#ifndef APP_CLI_OTA_H
#define APP_CLI_OTA_H

#include <stdbool.h>

void app_cli_suspend_for_ota(void);
void app_cli_resume_after_ota(void);

#ifdef HOST_TEST
void app_cli_test_reset(void);
bool app_cli_test_is_suspended_for_ota(void);
#endif

#endif /* APP_CLI_OTA_H */
