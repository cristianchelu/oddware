/*
 * Schedule command layer — spec/30-processes/web-ui.md, scheduler-engine.md
 */

#ifndef SCHEDULE_CMD_H
#define SCHEDULE_CMD_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mqtt_route.h"
#include "schedule.h"

typedef enum {
    SCHEDULE_CMD_OK = 0,
    SCHEDULE_CMD_INVALID,
    SCHEDULE_CMD_NOT_FOUND,
    SCHEDULE_CMD_NVDM_FAIL,
    SCHEDULE_CMD_UNCHANGED,
} schedule_cmd_result_t;

schedule_cmd_result_t schedule_cmd_set(const schedule_slot_config_t *cfg);
schedule_cmd_result_t schedule_cmd_delete(uint8_t hour, uint8_t min);
schedule_cmd_result_t schedule_cmd_toggle(uint8_t hour, uint8_t min);
schedule_cmd_result_t schedule_cmd_skip(uint8_t hour, uint8_t min, bool skip);
schedule_cmd_result_t schedule_cmd_enable(bool enabled);
schedule_cmd_result_t schedule_cmd_today(bool enabled);

bool schedule_cmd_apply_json(mqtt_route_kind_t route,
                             const char *json,
                             size_t len);

#endif /* SCHEDULE_CMD_H */
