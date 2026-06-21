/*
 * Schedule NVDM binary layouts — spec/30-processes/scheduler-engine.md
 */

#ifndef SCHEDULE_NVDM_H
#define SCHEDULE_NVDM_H

#include <stdint.h>

#include "schedule.h"

#define SCHEDULE_NVDM_CONFIG_MAGIC    0x49463153u  /* 'IF1S' */
#define SCHEDULE_NVDM_CONFIG_VERSION  1u

#define SCHEDULE_NVDM_SLOT_FLAG_ENABLED 0x01u

typedef struct __attribute__((packed)) {
    uint8_t hour;
    uint8_t min;
    uint8_t days;
    uint8_t g;
    uint8_t flags;
} schedule_nvdm_slot_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t  version;
    uint8_t  count;
    uint8_t  reserved[2];
    schedule_nvdm_slot_t slots[SCHEDULE_MAX_SLOTS];
} schedule_nvdm_config_t;

#define SCHEDULE_NVDM_RUNTIME_MAGIC    0x49463152u  /* 'IF1R' */
#define SCHEDULE_NVDM_RUNTIME_VERSION  1u

#define SCHEDULE_NVDM_RUNTIME_FLAG_SKIP_TODAY   0x01u
#define SCHEDULE_NVDM_RUNTIME_FLAG_FIRED_TODAY  0x02u
#define SCHEDULE_NVDM_RUNTIME_HDR_TODAY_ENABLED 0x01u

typedef struct __attribute__((packed)) {
    uint8_t  state;
    uint8_t  flags;
    int16_t  g_actual;
} schedule_nvdm_runtime_slot_t;

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint8_t  version;
    uint16_t local_yday;
    uint8_t  flags;
    uint8_t  count;
    schedule_nvdm_runtime_slot_t slots[SCHEDULE_MAX_SLOTS];
} schedule_nvdm_runtime_t;

#endif /* SCHEDULE_NVDM_H */
