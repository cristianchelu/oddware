#include <stddef.h>
#include <stdbool.h>
#include <string.h>

#include "fake_config_port.h"

#define FAKE_MAX_ENTRIES 16
#define FAKE_VALUE_LEN   128

typedef struct {
    char group[32];
    char key[32];
    char value[FAKE_VALUE_LEN];
} fake_entry_t;

static fake_entry_t s_entries[FAKE_MAX_ENTRIES];
static size_t s_entry_count;

static fake_entry_t *find_entry(const char *group, const char *key)
{
    size_t i;

    for (i = 0; i < s_entry_count; i++) {
        if (strcmp(s_entries[i].group, group) == 0 &&
            strcmp(s_entries[i].key, key) == 0) {
            return &s_entries[i];
        }
    }

    return NULL;
}

void fake_config_port_reset(void)
{
    s_entry_count = 0;
}

static port_err_t fake_read(const char *group, const char *key, char *buf, size_t len)
{
    const fake_entry_t *entry;

    if (group == NULL || key == NULL || buf == NULL || len == 0) {
        return PORT_ERR_INVALID_ARG;
    }

    entry = find_entry(group, key);
    if (entry == NULL) {
        return PORT_ERR_NOT_FOUND;
    }

    if (strlen(entry->value) + 1 > len) {
        return PORT_ERR_INVALID_ARG;
    }

    strcpy(buf, entry->value);
    return PORT_OK;
}

static port_err_t fake_write(const char *group, const char *key, const char *value)
{
    fake_entry_t *entry;

    if (group == NULL || key == NULL || value == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    entry = find_entry(group, key);
    if (entry == NULL) {
        if (s_entry_count >= FAKE_MAX_ENTRIES) {
            return PORT_ERR_IO;
        }

        entry = &s_entries[s_entry_count++];
        strncpy(entry->group, group, sizeof(entry->group) - 1);
        entry->group[sizeof(entry->group) - 1] = '\0';
        strncpy(entry->key, key, sizeof(entry->key) - 1);
        entry->key[sizeof(entry->key) - 1] = '\0';
    }

    strncpy(entry->value, value, sizeof(entry->value) - 1);
    entry->value[sizeof(entry->value) - 1] = '\0';
    return PORT_OK;
}

static port_err_t fake_erase(const char *group, const char *key)
{
    size_t i;

    if (group == NULL || key == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    for (i = 0; i < s_entry_count; i++) {
        if (strcmp(s_entries[i].group, group) == 0 &&
            strcmp(s_entries[i].key, key) == 0) {
            s_entry_count--;
            if (i < s_entry_count) {
                s_entries[i] = s_entries[s_entry_count];
            }
            return PORT_OK;
        }
    }

    return PORT_ERR_NOT_FOUND;
}

static port_err_t fake_erase_group(const char *group)
{
    size_t i = 0;
    bool removed = false;

    if (group == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    while (i < s_entry_count) {
        if (strcmp(s_entries[i].group, group) == 0) {
            s_entry_count--;
            if (i < s_entry_count) {
                s_entries[i] = s_entries[s_entry_count];
            }
            removed = true;
            continue;
        }
        i++;
    }

    return PORT_OK;
}

static const config_port_t s_fake_port = {
    .read = fake_read,
    .write = fake_write,
    .erase = fake_erase,
    .erase_group = fake_erase_group,
};

const config_port_t *fake_config_port_get(void)
{
    return &s_fake_port;
}
