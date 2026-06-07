/*
 * UART CLI: Wi-Fi commands — spec/30-processes/uart-console.md
 */

#include <stdio.h>
#include <string.h>

#include "cli.h"

#include "config_keys.h"
#include "config_port.h"
#include "port_err.h"
#include "wifi_cred.h"
#include "app_wifi_cli.h"
#include "wifi_sta.h"

static const char *wifi_cli_pass_display(bool ssid_set, port_err_t pass_err, const char *pass)
{
    if (!ssid_set) {
        return "(unset)";
    }

    if (pass_err == PORT_ERR_NOT_FOUND || wifi_cred_is_open_network(pass)) {
        return "(open)";
    }

    return "********";
}

static uint8_t wifi_cli_show(uint8_t argc, char *argv[])
{
    const config_port_t *cfg = config_port_get();
    char ssid[WIFI_SSID_MAX_LEN + 1];
    char pass[WIFI_PASS_MAX_LEN + 1];
    port_err_t pass_err;

    (void)argc;
    (void)argv;

    if (cfg->read(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_SSID, ssid, sizeof(ssid)) != PORT_OK) {
        ssid[0] = '\0';
    }

    pass_err = cfg->read(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_PASS, pass, sizeof(pass));
    if (pass_err != PORT_OK) {
        pass[0] = '\0';
    }

    printf("ssid: %s\r\n", ssid[0] ? ssid : "(unset)");
    printf("pass: %s\r\n", wifi_cli_pass_display(ssid[0] != '\0', pass_err, pass));
    return 0;
}

static uint8_t wifi_cli_set_ssid(uint8_t argc, char *argv[])
{
    const config_port_t *cfg = config_port_get();

    if (argc < 1 || argv[0] == NULL) {
        printf("usage: wifi set ssid <name>\r\n");
        return 1;
    }

    if (wifi_cred_validate(argv[0], "") != PORT_OK) {
        printf("invalid ssid\r\n");
        return 1;
    }

    if (cfg->write(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_SSID, argv[0]) != PORT_OK) {
        printf("nvdm write failed\r\n");
        return 1;
    }

    printf("ssid saved\r\n");
    return 0;
}

static uint8_t wifi_cli_set_pass(uint8_t argc, char *argv[])
{
    const config_port_t *cfg = config_port_get();
    const char *pass = "";

    if (argc < 1 || argv[0] == NULL) {
        printf("usage: wifi set pass <password>\r\n");
        return 1;
    }

    pass = argv[0];
    if (wifi_cred_validate("x", pass) != PORT_OK) {
        printf("invalid password\r\n");
        return 1;
    }

    if (cfg->write(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_PASS, pass) != PORT_OK) {
        printf("nvdm write failed\r\n");
        return 1;
    }

    printf("password saved\r\n");
    return 0;
}

static uint8_t wifi_cli_set(uint8_t argc, char *argv[])
{
    if (argc < 1 || argv[0] == NULL) {
        printf("usage: wifi set ssid|pass <value>\r\n");
        return 1;
    }

    if (strcmp(argv[0], "ssid") == 0) {
        return wifi_cli_set_ssid(argc - 1, argv + 1);
    }

    if (strcmp(argv[0], "pass") == 0) {
        return wifi_cli_set_pass(argc - 1, argv + 1);
    }

    printf("usage: wifi set ssid|pass <value>\r\n");
    return 1;
}

static uint8_t wifi_cli_connect(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (!wifi_cred_is_stored(config_port_get())) {
        printf("set ssid first\r\n");
        return 1;
    }

    if (!wifi_sta_request_connect()) {
        printf("connect already in progress\r\n");
        return 1;
    }

    printf("connecting...\r\n");
    return 0;
}

cmd_t wifi_cli_subcmds[] = {
    { "show",    "show stored credentials", wifi_cli_show,    NULL },
    { "set",     "set ssid|pass",           wifi_cli_set,     NULL },
    { "connect", "connect using NVDM",      wifi_cli_connect, NULL },
    { NULL, NULL, NULL, NULL },
};
