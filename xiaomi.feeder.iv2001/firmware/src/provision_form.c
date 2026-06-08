/*
 * Captive-portal form parsing and HTML — spec/30-processes/provisioning-flow.md
 */

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "provision_form.h"

static int hex_nibble(char c)
{
    if (c >= '0' && c <= '9') {
        return c - '0';
    }
    if (c >= 'A' && c <= 'F') {
        return c - 'A' + 10;
    }
    if (c >= 'a' && c <= 'f') {
        return c - 'a' + 10;
    }
    return -1;
}

static void url_decode(const char *src, char *dst, size_t dst_len)
{
    size_t di = 0;

    if (src == NULL || dst == NULL || dst_len == 0) {
        return;
    }

    while (*src != '\0' && di + 1 < dst_len) {
        if (*src == '+') {
            dst[di++] = ' ';
            src++;
            continue;
        }

        if (*src == '%' && isxdigit((unsigned char)src[1]) && isxdigit((unsigned char)src[2])) {
            int hi = hex_nibble(src[1]);
            int lo = hex_nibble(src[2]);

            if (hi >= 0 && lo >= 0) {
                dst[di++] = (char)((hi << 4) | lo);
                src += 3;
                continue;
            }
        }

        dst[di++] = *src++;
    }

    dst[di] = '\0';
}

static void set_field(provision_input_t *out, const char *key, const char *value)
{
    char decoded[MQTT_HOST_MAX_LEN + 1];

    if (out == NULL || key == NULL || value == NULL) {
        return;
    }

    url_decode(value, decoded, sizeof(decoded));

    if (strcmp(key, PROVISION_FIELD_WIFI_SSID) == 0) {
        strncpy(out->wifi_ssid, decoded, sizeof(out->wifi_ssid) - 1);
        out->wifi_ssid[sizeof(out->wifi_ssid) - 1] = '\0';
    } else if (strcmp(key, PROVISION_FIELD_WIFI_PASS) == 0) {
        strncpy(out->wifi_pass, decoded, sizeof(out->wifi_pass) - 1);
        out->wifi_pass[sizeof(out->wifi_pass) - 1] = '\0';
    } else if (strcmp(key, PROVISION_FIELD_MQTT_HOST) == 0) {
        strncpy(out->mqtt_host, decoded, sizeof(out->mqtt_host) - 1);
        out->mqtt_host[sizeof(out->mqtt_host) - 1] = '\0';
    } else if (strcmp(key, PROVISION_FIELD_MQTT_PORT) == 0) {
        unsigned long port_val = strtoul(decoded, NULL, 10);

        if (decoded[0] != '\0') {
            out->mqtt_port_set = true;
            if (port_val >= 1 && port_val <= 65535) {
                out->mqtt_port = (uint16_t)port_val;
            }
        }
    } else if (strcmp(key, PROVISION_FIELD_MQTT_USER) == 0) {
        strncpy(out->mqtt_user, decoded, sizeof(out->mqtt_user) - 1);
        out->mqtt_user[sizeof(out->mqtt_user) - 1] = '\0';
    } else if (strcmp(key, PROVISION_FIELD_MQTT_PASS) == 0) {
        strncpy(out->mqtt_pass, decoded, sizeof(out->mqtt_pass) - 1);
        out->mqtt_pass[sizeof(out->mqtt_pass) - 1] = '\0';
    } else if (strcmp(key, PROVISION_FIELD_DEVICE_ID) == 0) {
        strncpy(out->device_id, decoded, sizeof(out->device_id) - 1);
        out->device_id[sizeof(out->device_id) - 1] = '\0';
    }
}

void provision_input_init(provision_input_t *input)
{
    if (input == NULL) {
        return;
    }

    memset(input, 0, sizeof(*input));
    input->mqtt_port = 1883;
}

port_err_t provision_form_parse_urlencoded(const char *body,
                                           size_t body_len,
                                           provision_input_t *out)
{
    char pair[512];
    char *eq;
    size_t i;
    size_t start = 0;

    if (body == NULL || out == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    provision_input_init(out);

    for (i = 0; i <= body_len; i++) {
        if (i < body_len && body[i] != '&') {
            continue;
        }

        size_t chunk_len = i - start;

        if (chunk_len == 0) {
            start = i + 1;
            continue;
        }

        if (chunk_len >= sizeof(pair)) {
            return PORT_ERR_INVALID_ARG;
        }

        memcpy(pair, body + start, chunk_len);
        pair[chunk_len] = '\0';

        eq = strchr(pair, '=');
        if (eq == NULL) {
            set_field(out, pair, "");
        } else {
            *eq = '\0';
            set_field(out, pair, eq + 1);
        }

        start = i + 1;
    }

    return PORT_OK;
}

port_err_t provision_form_validate(const provision_input_t *input)
{
    port_err_t err;

    if (input == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = wifi_cred_validate(input->wifi_ssid, input->wifi_pass);
    if (err != PORT_OK) {
        return err;
    }

    err = mqtt_cred_validate_host(input->mqtt_host);
    if (err != PORT_OK) {
        return err;
    }

    if (input->mqtt_port_set) {
        err = mqtt_cred_validate_port(input->mqtt_port);
        if (err != PORT_OK) {
            return err;
        }
    }

    err = mqtt_cred_validate_device_id(input->device_id);
    if (err != PORT_OK) {
        return err;
    }

    if (strlen(input->mqtt_user) > MQTT_USER_MAX_LEN) {
        return PORT_ERR_INVALID_ARG;
    }

    if (strlen(input->mqtt_pass) > MQTT_PASS_MAX_LEN) {
        return PORT_ERR_INVALID_ARG;
    }

    return PORT_OK;
}

static void html_escape(const char *src, char *dst, size_t dst_len)
{
    size_t di = 0;

    if (src == NULL || dst == NULL || dst_len == 0) {
        return;
    }

    for (; *src != '\0' && di + 1 < dst_len; src++) {
        const char *rep = NULL;

        switch (*src) {
        case '&':
            rep = "&amp;";
            break;
        case '<':
            rep = "&lt;";
            break;
        case '>':
            rep = "&gt;";
            break;
        case '"':
            rep = "&quot;";
            break;
        default:
            dst[di++] = *src;
            continue;
        }

        if (rep != NULL) {
            size_t rep_len = strlen(rep);

            if (di + rep_len >= dst_len) {
                break;
            }
            memcpy(dst + di, rep, rep_len);
            di += rep_len;
        }
    }

    dst[di] = '\0';
}

size_t provision_form_render(const provision_input_t *input,
                             const char *message,
                             char *buf,
                             size_t len)
{
    char field_esc[MQTT_HOST_MAX_LEN * 6 + 1];
    char msg_esc[256];
    char port_buf[8];
    char msg_block[320];
    const provision_input_t empty = {0};
    const provision_input_t *fields = (input != NULL) ? input : &empty;
    int written;

    if (buf == NULL || len == 0) {
        return 0;
    }

    if (fields->mqtt_port_set) {
        snprintf(port_buf, sizeof(port_buf), "%u", (unsigned)fields->mqtt_port);
    } else {
        port_buf[0] = '\0';
    }

    if (message != NULL && message[0] != '\0') {
        html_escape(message, msg_esc, sizeof(msg_esc));
        snprintf(msg_block, sizeof(msg_block), "<p><strong>%s</strong></p>", msg_esc);
    } else {
        msg_block[0] = '\0';
    }

    html_escape(fields->wifi_ssid, field_esc, sizeof(field_esc));
    written = snprintf(buf, len,
                         "<!DOCTYPE html><html><head><title>PetFeeder Setup</title></head><body>"
                         "<h1>PetFeeder Setup</h1>"
                         "%s"
                         "<form method=\"post\" action=\"/provision.cgi\">"
                         "<p>Wi-Fi SSID<br><input name=\"wifi_ssid\" value=\"%s\" maxlength=\"32\"></p>",
                         msg_block,
                         field_esc);
    if (written <= 0 || (size_t)written >= len) {
        return 0;
    }

    html_escape(fields->mqtt_host, field_esc, sizeof(field_esc));
    written += snprintf(buf + written, len - (size_t)written,
                        "<p>Wi-Fi password (empty = open)<br><input type=\"password\" name=\"wifi_pass\" value=\"\"></p>"
                        "<p>MQTT broker host<br><input name=\"mqtt_host\" value=\"%s\" maxlength=\"253\"></p>"
                        "<p>MQTT broker port<br><input name=\"mqtt_port\" value=\"%s\" maxlength=\"5\"></p>",
                        field_esc,
                        port_buf);
    if (written <= 0 || (size_t)written >= len) {
        return 0;
    }

    html_escape(fields->mqtt_user, field_esc, sizeof(field_esc));
    written += snprintf(buf + written, len - (size_t)written,
                        "<p>MQTT username<br><input name=\"mqtt_user\" value=\"%s\" maxlength=\"64\"></p>",
                        field_esc);
    if (written <= 0 || (size_t)written >= len) {
        return 0;
    }

    html_escape(fields->device_id, field_esc, sizeof(field_esc));
    written += snprintf(buf + written, len - (size_t)written,
                        "<p>MQTT password<br><input type=\"password\" name=\"mqtt_pass\" value=\"\"></p>"
                        "<p>Device ID (optional)<br><input name=\"device_id\" value=\"%s\" maxlength=\"32\"></p>"
                        "<p><input type=\"submit\" value=\"Save and connect\"></p>"
                        "</form></body></html>",
                        field_esc);

    if (written <= 0 || (size_t)written >= len) {
        return 0;
    }

    return (size_t)written;
}

size_t provision_form_render_success(char *buf, size_t len)
{
    int written;

    if (buf == NULL || len == 0) {
        return 0;
    }

    written = snprintf(buf, len,
                       "<!DOCTYPE html><html><head>"
                       "<meta http-equiv=\"refresh\" content=\"3;url=/\">"
                       "<title>PetFeeder Setup</title></head><body>"
                       "<h1>PetFeeder Setup</h1>"
                       "<p>%s</p>"
                       "</body></html>",
                       PROVISION_MSG_SUCCESS);
    if (written <= 0 || (size_t)written >= len) {
        return 0;
    }

    return (size_t)written;
}
