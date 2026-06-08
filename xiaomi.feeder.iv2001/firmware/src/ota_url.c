/*
 * OTA URL validation and MQTT cmd parsing — spec/30-processes/ota-flow.md
 */

#include <stdio.h>
#include <ctype.h>
#include <string.h>

#include "ota_url.h"

static bool ota_json_find_string(const char *json,
                                 size_t len,
                                 const char *key,
                                 char *out,
                                 size_t out_len)
{
    char pattern[32];
    const char *cursor;
    const char *end;
    const char *value_start;
    const char *value_end;
    size_t value_len;

    if (json == NULL || key == NULL || out == NULL || out_len == 0) {
        return false;
    }

    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    end = json + len;
    cursor = json;

    while (cursor < end) {
        const char *found = strstr(cursor, pattern);

        if (found == NULL || found >= end) {
            return false;
        }

        cursor = found + strlen(pattern);
        while (cursor < end && (*cursor == ' ' || *cursor == '\t' ||
                                *cursor == '\r' || *cursor == '\n')) {
            cursor++;
        }
        if (cursor >= end || *cursor != ':') {
            continue;
        }
        cursor++;
        while (cursor < end && (*cursor == ' ' || *cursor == '\t' ||
                                *cursor == '\r' || *cursor == '\n')) {
            cursor++;
        }
        if (cursor >= end || *cursor != '"') {
            return false;
        }
        cursor++;
        value_start = cursor;
        while (cursor < end && *cursor != '"') {
            cursor++;
        }
        if (cursor >= end) {
            return false;
        }
        value_end = cursor;

        value_len = (size_t)(value_end - value_start);
        if (value_len + 1 > out_len) {
            return false;
        }

        memcpy(out, value_start, value_len);
        out[value_len] = '\0';
        return true;
    }

    return false;
}

static port_err_t ota_hex_nibble(char c, uint8_t *out)
{
    if (c >= '0' && c <= '9') {
        *out = (uint8_t)(c - '0');
        return PORT_OK;
    }
    if (c >= 'a' && c <= 'f') {
        *out = (uint8_t)(c - 'a' + 10);
        return PORT_OK;
    }
    if (c >= 'A' && c <= 'F') {
        *out = (uint8_t)(c - 'A' + 10);
        return PORT_OK;
    }
    return PORT_ERR_INVALID_ARG;
}

port_err_t ota_url_validate(const char *url)
{
    size_t len;

    if (url == NULL || url[0] == '\0') {
        return PORT_ERR_INVALID_ARG;
    }

    len = strlen(url);
    if (len > OTA_URL_MAX_LEN) {
        return PORT_ERR_INVALID_ARG;
    }

    if (strncmp(url, "http://", 7) == 0) {
        return PORT_OK;
    }
    if (strncmp(url, "https://", 8) == 0) {
        return PORT_OK;
    }

    return PORT_ERR_INVALID_ARG;
}

port_err_t ota_cmd_parse(const char *payload,
                         size_t len,
                         char *url_out,
                         size_t url_out_len,
                         uint8_t sha512_out[FLASH_BANK_SHA512_LEN],
                         bool *has_sha512_out)
{
    char sha_hex[OTA_SHA512_HEX_LEN + 1];
    size_t i;

    if (payload == NULL || url_out == NULL || has_sha512_out == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    *has_sha512_out = false;
    url_out[0] = '\0';

    if (!ota_json_find_string(payload, len, "url", url_out, url_out_len)) {
        return PORT_ERR_INVALID_ARG;
    }

    if (ota_url_validate(url_out) != PORT_OK) {
        return PORT_ERR_INVALID_ARG;
    }

    if (sha512_out == NULL) {
        return PORT_OK;
    }

    if (!ota_json_find_string(payload, len, "sha512", sha_hex, sizeof(sha_hex))) {
        return PORT_OK;
    }

    if (strlen(sha_hex) != OTA_SHA512_HEX_LEN) {
        return PORT_ERR_INVALID_ARG;
    }

    for (i = 0; i < FLASH_BANK_SHA512_LEN; i++) {
        uint8_t hi;
        uint8_t lo;

        if (ota_hex_nibble(sha_hex[i * 2], &hi) != PORT_OK ||
            ota_hex_nibble(sha_hex[i * 2 + 1], &lo) != PORT_OK) {
            return PORT_ERR_INVALID_ARG;
        }

        sha512_out[i] = (uint8_t)((hi << 4) | lo);
    }

    *has_sha512_out = true;
    return PORT_OK;
}
