/*
 * Config port — spec/40-architecture/ports.md
 */

#ifndef CONFIG_PORT_H
#define CONFIG_PORT_H

#include <stddef.h>

#include "port_err.h"

typedef struct config_port {
    port_err_t (*read)(const char *group, const char *key, char *buf, size_t len);
    port_err_t (*write)(const char *group, const char *key, const char *value);
    port_err_t (*read_blob)(const char *group,
                            const char *key,
                            void *buf,
                            size_t len,
                            size_t *out_len);
    port_err_t (*write_blob)(const char *group,
                             const char *key,
                             const void *data,
                             size_t len);
    port_err_t (*erase)(const char *group, const char *key);
    port_err_t (*erase_group)(const char *group);
} config_port_t;

const config_port_t *config_port_get(void);

#endif /* CONFIG_PORT_H */
