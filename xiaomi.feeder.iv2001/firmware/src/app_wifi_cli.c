/*
 * UART CLI: Wi-Fi commands — spec/30-processes/uart-console.md
 */

#include <stdio.h>
#include "app_log.h"
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

    app_log_info("cli", "ssid: %s", ssid[0] ? ssid : "(unset)");
    app_log_info("cli", "pass: %s", wifi_cli_pass_display(ssid[0] != '\0', pass_err, pass));
    return 0;
}

static uint8_t wifi_cli_set_ssid(uint8_t argc, char *argv[])
{
    const config_port_t *cfg = config_port_get();

    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: wifi set ssid <name>");
        return 1;
    }

    if (wifi_cred_validate(argv[0], "") != PORT_OK) {
        app_log_info("cli", "invalid ssid");
        return 1;
    }

    if (cfg->write(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_SSID, argv[0]) != PORT_OK) {
        app_log_info("cli", "nvdm write failed");
        return 1;
    }

    app_log_info("cli", "ssid saved");
    return 0;
}

static uint8_t wifi_cli_set_pass(uint8_t argc, char *argv[])
{
    const config_port_t *cfg = config_port_get();
    const char *pass = "";

    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: wifi set pass <password>");
        return 1;
    }

    pass = argv[0];
    if (wifi_cred_validate("x", pass) != PORT_OK) {
        app_log_info("cli", "invalid password");
        return 1;
    }

    if (cfg->write(CONFIG_GROUP_WIFI, CONFIG_KEY_WIFI_PASS, pass) != PORT_OK) {
        app_log_info("cli", "nvdm write failed");
        return 1;
    }

    app_log_info("cli", "password saved");
    return 0;
}

static uint8_t wifi_cli_set(uint8_t argc, char *argv[])
{
    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: wifi set ssid|pass <value>");
        return 1;
    }

    if (strcmp(argv[0], "ssid") == 0) {
        return wifi_cli_set_ssid(argc - 1, argv + 1);
    }

    if (strcmp(argv[0], "pass") == 0) {
        return wifi_cli_set_pass(argc - 1, argv + 1);
    }

    app_log_info("cli", "usage: wifi set ssid|pass <value>");
    return 1;
}

static uint8_t wifi_cli_connect(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (!wifi_cred_is_stored(config_port_get())) {
        app_log_info("cli", "set ssid first");
        return 1;
    }

    if (!wifi_sta_request_connect()) {
        app_log_info("cli", "connect already in progress");
        return 1;
    }

    app_log_info("cli", "connecting...");
    return 0;
}

static uint8_t wifi_cli_disconnect(uint8_t argc, char *argv[])
{
    wifi_sta_busy_t busy;

    (void)argc;
    (void)argv;

    busy = wifi_sta_busy();
    if (busy == WIFI_STA_BUSY_CONNECT) {
        app_log_info("cli", "connect already in progress");
        return 1;
    }
    if (busy == WIFI_STA_BUSY_DISCONNECT) {
        app_log_info("cli", "disconnect already in progress");
        return 1;
    }

    if (!wifi_sta_request_disconnect()) {
        app_log_info("cli", "disconnect already in progress");
        return 1;
    }

    app_log_info("cli", "disconnecting...");
    return 0;
}

uint8_t wifi_cli_run_disconnect(void)
{
    return wifi_cli_disconnect(0, NULL);
}

cmd_t wifi_cli_subcmds[] = {
    { "show",       "show stored credentials", wifi_cli_show,       NULL },
    { "set",        "set ssid|pass",           wifi_cli_set,        NULL },
    { "connect",    "connect using NVDM",      wifi_cli_connect,    NULL },
    { "disconnect", "tear down STA session",   wifi_cli_disconnect, NULL },
    { NULL, NULL, NULL, NULL },
};
