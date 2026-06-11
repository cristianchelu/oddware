/*
 * UART CLI: MQTT commands — spec/30-processes/uart-console.md
 */

#include <stdio.h>
#include "app_log.h"
#include <stdlib.h>
#include <string.h>

#include "cli.h"

#include "config_keys.h"
#include "config_port.h"
#include "mqtt_cred.h"
#include "mqtt_client.h"
#include "app_mqtt_cli.h"

static uint8_t mqtt_cli_show(uint8_t argc, char *argv[])
{
    const config_port_t *cfg = config_port_get();
    char host[MQTT_HOST_MAX_LEN + 1];
    char port_buf[8];
    char user[MQTT_USER_MAX_LEN + 1];
    char pass[MQTT_PASS_MAX_LEN + 1];
    char device_id[MQTT_DEVICE_ID_MAX_LEN + 1];
    char tls_buf[8];
    port_err_t err;

    (void)argc;
    (void)argv;

    if (cfg->read(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_HOST, host, sizeof(host)) != PORT_OK) {
        host[0] = '\0';
    }

    err = cfg->read(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_PORT, port_buf, sizeof(port_buf));
    if (err != PORT_OK) {
        port_buf[0] = '\0';
    }

    err = cfg->read(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_USER, user, sizeof(user));
    if (err != PORT_OK) {
        user[0] = '\0';
    }

    err = cfg->read(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_PASS, pass, sizeof(pass));
    if (err != PORT_OK) {
        pass[0] = '\0';
    }

    err = cfg->read(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_DEVICE_ID, device_id, sizeof(device_id));
    if (err != PORT_OK) {
        device_id[0] = '\0';
    }

    err = cfg->read(CONFIG_GROUP_MQTT, CONFIG_KEY_MQTT_TLS, tls_buf, sizeof(tls_buf));
    if (err != PORT_OK) {
        strcpy(tls_buf, "false");
    }

    app_log_info("cli", "host: %s", host[0] ? host : "(unset)");
    app_log_info("cli", "port: %s", port_buf[0] ? port_buf : "1883");
    app_log_info("cli", "user: %s", user[0] ? user : "(anonymous)");
    app_log_info("cli", "pass: %s", pass[0] ? "********" : "(empty)");
    app_log_info("cli", "device_id: %s", device_id[0] ? device_id : "(mac)");
    app_log_info("cli", "tls: %s", tls_buf);
    return 0;
}

static uint8_t mqtt_cli_set_host(uint8_t argc, char *argv[])
{
    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: mqtt set host <hostname>");
        return 1;
    }

    if (mqtt_cred_save_host(config_port_get(), argv[0]) != PORT_OK) {
        app_log_info("cli", "invalid host");
        return 1;
    }

    app_log_info("cli", "host saved");
    return 0;
}

static uint8_t mqtt_cli_set_port(uint8_t argc, char *argv[])
{
    unsigned long port_val;
    char *end;

    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: mqtt set port <port>");
        return 1;
    }

    port_val = strtoul(argv[0], &end, 10);
    if (end == argv[0] || *end != '\0' || port_val > 65535) {
        app_log_info("cli", "invalid port");
        return 1;
    }

    if (mqtt_cred_save_port(config_port_get(), (uint16_t)port_val) != PORT_OK) {
        app_log_info("cli", "invalid port");
        return 1;
    }

    app_log_info("cli", "port saved");
    return 0;
}

static uint8_t mqtt_cli_set_user(uint8_t argc, char *argv[])
{
    const char *user = "";

    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: mqtt set user <username>");
        return 1;
    }

    user = argv[0];
    if (mqtt_cred_save_user(config_port_get(), user) != PORT_OK) {
        app_log_info("cli", "nvdm write failed");
        return 1;
    }

    app_log_info("cli", "user saved");
    return 0;
}

static uint8_t mqtt_cli_set_pass(uint8_t argc, char *argv[])
{
    const char *pass = "";

    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: mqtt set pass <password>");
        return 1;
    }

    pass = argv[0];
    if (mqtt_cred_save_pass(config_port_get(), pass) != PORT_OK) {
        app_log_info("cli", "nvdm write failed");
        return 1;
    }

    app_log_info("cli", "password saved");
    return 0;
}

static uint8_t mqtt_cli_set_device_id(uint8_t argc, char *argv[])
{
    const char *device_id = "";

    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: mqtt set device_id <id>");
        return 1;
    }

    device_id = argv[0];
    if (mqtt_cred_save_device_id(config_port_get(), device_id) != PORT_OK) {
        app_log_info("cli", "invalid device_id");
        return 1;
    }

    app_log_info("cli", "device_id saved");
    return 0;
}

static uint8_t mqtt_cli_set(uint8_t argc, char *argv[])
{
    if (argc < 1 || argv[0] == NULL) {
        app_log_info("cli", "usage: mqtt set host|port|user|pass|device_id <value>");
        return 1;
    }

    if (strcmp(argv[0], "host") == 0) {
        return mqtt_cli_set_host(argc - 1, argv + 1);
    }
    if (strcmp(argv[0], "port") == 0) {
        return mqtt_cli_set_port(argc - 1, argv + 1);
    }
    if (strcmp(argv[0], "user") == 0) {
        return mqtt_cli_set_user(argc - 1, argv + 1);
    }
    if (strcmp(argv[0], "pass") == 0) {
        return mqtt_cli_set_pass(argc - 1, argv + 1);
    }
    if (strcmp(argv[0], "device_id") == 0) {
        return mqtt_cli_set_device_id(argc - 1, argv + 1);
    }

    app_log_info("cli", "usage: mqtt set host|port|user|pass|device_id <value>");
    return 1;
}

static uint8_t mqtt_cli_disconnect(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    mqtt_client_stop();
    app_log_info("cli", "mqtt stopped");
    return 0;
}

static uint8_t mqtt_cli_connect(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (!mqtt_cred_is_stored(config_port_get())) {
        app_log_info("cli", "set host first");
        return 1;
    }

    if (mqtt_client_connect_in_progress()) {
        app_log_info("cli", "connect already in progress");
        return 1;
    }

    if (!mqtt_client_wifi_is_ready()) {
        app_log_info("cli", "wifi not ready");
        return 1;
    }

    if (!mqtt_client_request_connect()) {
        app_log_info("cli", "connect already in progress");
        return 1;
    }

    app_log_info("cli", "connecting...");
    return 0;
}

cmd_t mqtt_cli_subcmds[] = {
    { "show",       "show broker settings", mqtt_cli_show,       NULL },
    { "set",        "set host|port|...",    mqtt_cli_set,        NULL },
    { "connect",    "connect to broker",    mqtt_cli_connect,    NULL },
    { "disconnect", "stop mqtt client",     mqtt_cli_disconnect, NULL },
    { NULL, NULL, NULL, NULL },
};
