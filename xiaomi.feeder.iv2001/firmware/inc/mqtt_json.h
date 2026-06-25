/*
 * Minimal JSON field helpers for MQTT payloads.
 */

#ifndef MQTT_JSON_H
#define MQTT_JSON_H

#include <stdbool.h>
#include <stddef.h>

bool mqtt_json_has_key(const char *json, size_t len, const char *key);
bool mqtt_json_find_string(const char *json,
                            size_t len,
                            const char *key,
                            char *out,
                            size_t out_len);
bool mqtt_json_find_uint(const char *json,
                          size_t len,
                          const char *key,
                          unsigned *out);

#endif /* MQTT_JSON_H */
