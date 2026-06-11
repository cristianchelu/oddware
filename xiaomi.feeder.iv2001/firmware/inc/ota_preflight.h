/*
 * OTA pre-download memory reclaim — spec/30-processes/ota-flow.md
 */

#ifndef OTA_PREFLIGHT_H
#define OTA_PREFLIGHT_H

#include "port_err.h"

port_err_t ota_preflight_suspend_idle_tasks(void);
void ota_preflight_resume_idle_tasks(void);

#endif /* OTA_PREFLIGHT_H */
