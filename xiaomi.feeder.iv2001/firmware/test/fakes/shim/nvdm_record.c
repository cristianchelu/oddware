#include <string.h>

#include "nvdm_record.h"

static nvdm_record_entry_t s_records[NVDM_RECORD_MAX];
static size_t s_record_count;

void nvdm_record_reset(void)
{
    s_record_count = 0u;
    memset(s_records, 0, sizeof(s_records));
}

void nvdm_record_write(const char *group,
                       const char *key,
                       const uint8_t *data,
                       uint32_t len)
{
    nvdm_record_entry_t *entry;

    if (group == NULL || key == NULL || data == NULL) {
        return;
    }

    if (s_record_count >= NVDM_RECORD_MAX) {
        return;
    }

    entry = &s_records[s_record_count++];
    strncpy(entry->group, group, sizeof(entry->group) - 1u);
    entry->group[sizeof(entry->group) - 1u] = '\0';
    strncpy(entry->key, key, sizeof(entry->key) - 1u);
    entry->key[sizeof(entry->key) - 1u] = '\0';

    if (len > sizeof(entry->data)) {
        len = (uint32_t)sizeof(entry->data);
    }

    entry->len = len;
    memcpy(entry->data, data, len);
}

size_t nvdm_record_count(void)
{
    return s_record_count;
}

const nvdm_record_entry_t *nvdm_record_get(size_t index)
{
    if (index >= s_record_count) {
        return NULL;
    }

    return &s_records[index];
}
