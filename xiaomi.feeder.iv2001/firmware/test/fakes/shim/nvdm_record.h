#ifndef NVDM_RECORD_H
#define NVDM_RECORD_H

#include <stddef.h>
#include <stdint.h>

#define NVDM_RECORD_MAX 16

typedef struct {
    char group[16];
    char key[16];
    uint8_t data[64];
    uint32_t len;
} nvdm_record_entry_t;

void nvdm_record_reset(void);
void nvdm_record_write(const char *group,
                       const char *key,
                       const uint8_t *data,
                       uint32_t len);
size_t nvdm_record_count(void);
const nvdm_record_entry_t *nvdm_record_get(size_t index);

#endif /* NVDM_RECORD_H */
