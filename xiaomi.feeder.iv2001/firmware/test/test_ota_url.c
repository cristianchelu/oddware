/* Tests: spec/30-processes/ota-flow.md */

#include <stdio.h>
#include <string.h>

#include "unity.h"

#include "ota_url.h"

void test_ota_url_accepts_http(void)
{
    TEST_ASSERT_EQUAL(PORT_OK, ota_url_validate("http://192.168.1.10:8080/firmware.bin"));
}

void test_ota_url_accepts_https(void)
{
    TEST_ASSERT_EQUAL(PORT_OK, ota_url_validate("https://example.com/fw.bin"));
}

void test_ota_url_rejects_empty(void)
{
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, ota_url_validate(""));
}

void test_ota_url_rejects_ftp(void)
{
    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, ota_url_validate("ftp://host/fw.bin"));
}

void test_ota_cmd_parse_url_only(void)
{
    char url[128];
    bool has_sha512 = true;

    TEST_ASSERT_EQUAL(PORT_OK,
                      ota_cmd_parse("{\"url\":\"http://10.0.0.5/fw.bin\"}",
                                    strlen("{\"url\":\"http://10.0.0.5/fw.bin\"}"),
                                    url,
                                    sizeof(url),
                                    NULL,
                                    &has_sha512));
    TEST_ASSERT_FALSE(has_sha512);
    TEST_ASSERT_EQUAL_STRING("http://10.0.0.5/fw.bin", url);
}

void test_ota_cmd_parse_with_sha512(void)
{
    char url[128];
    uint8_t sha512[64];
    bool has_sha512 = false;
    char hex[OTA_SHA512_HEX_LEN + 1];
    char payload[320];
    int i;

    for (i = 0; i < OTA_SHA512_HEX_LEN; i++) {
        hex[i] = "0123456789abcdef"[i % 16];
    }
    hex[OTA_SHA512_HEX_LEN] = '\0';

    snprintf(payload,
             sizeof(payload),
             "{\"url\":\"http://10.0.0.5/fw.bin\",\"sha512\":\"%s\"}",
             hex);

    TEST_ASSERT_EQUAL(PORT_OK,
                      ota_cmd_parse(payload, strlen(payload), url, sizeof(url), sha512, &has_sha512));
    TEST_ASSERT_TRUE(has_sha512);
    TEST_ASSERT_EQUAL_HEX8(0x01, sha512[0]);
    TEST_ASSERT_EQUAL_HEX8(0x23, sha512[1]);
}

void test_ota_cmd_parse_invalid_url_scheme(void)
{
    char url[128];
    bool has_sha512 = false;

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      ota_cmd_parse("{\"url\":\"file:///tmp/fw.bin\"}",
                                    strlen("{\"url\":\"file:///tmp/fw.bin\"}"),
                                    url,
                                    sizeof(url),
                                    NULL,
                                    &has_sha512));
}

void test_ota_cmd_parse_sha512_wrong_length(void)
{
    char url[128];
    uint8_t sha512[64];
    bool has_sha512 = false;
    char hex[127];
    char payload[320];
    int i;

    for (i = 0; i < (int)sizeof(hex); i++) {
        hex[i] = 'a';
    }

    snprintf(payload,
             sizeof(payload),
             "{\"url\":\"http://10.0.0.5/fw.bin\",\"sha512\":\"%.*s\"}",
             (int)sizeof(hex),
             hex);

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      ota_cmd_parse(payload, strlen(payload), url, sizeof(url), sha512, &has_sha512));
}

void test_ota_cmd_parse_sha512_invalid_hex(void)
{
    char url[128];
    uint8_t sha512[64];
    bool has_sha512 = false;
    char hex[OTA_SHA512_HEX_LEN + 1];
    char payload[320];
    int i;

    for (i = 0; i < OTA_SHA512_HEX_LEN; i++) {
        hex[i] = 'a';
    }
    hex[0] = 'g';
    hex[OTA_SHA512_HEX_LEN] = '\0';

    snprintf(payload,
             sizeof(payload),
             "{\"url\":\"http://10.0.0.5/fw.bin\",\"sha512\":\"%s\"}",
             hex);

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      ota_cmd_parse(payload, strlen(payload), url, sizeof(url), sha512, &has_sha512));
}

void test_ota_cmd_parse_missing_url_key(void)
{
    char url[128];
    bool has_sha512 = false;
    const char *payload = "{\"sha512\":\"aa\"}";

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG,
                      ota_cmd_parse(payload, strlen(payload), url, sizeof(url), NULL, &has_sha512));
}

void test_ota_url_rejects_too_long(void)
{
    char url[OTA_URL_MAX_LEN + 16];
    int i;

    memcpy(url, "http://", 7);
    for (i = 7; i < OTA_URL_MAX_LEN + 1; i++) {
        url[i] = 'a';
    }
    url[OTA_URL_MAX_LEN + 1] = '\0';

    TEST_ASSERT_EQUAL(PORT_ERR_INVALID_ARG, ota_url_validate(url));
}
