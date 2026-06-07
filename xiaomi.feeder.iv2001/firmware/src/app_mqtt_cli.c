/*
 * UART CLI: MQTT commands — spec/30-processes/uart-console.md
 */

#include <stdio.h>
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

    printf("host: %s\r\n", host[0] ? host : "(unset)");
    printf("port: %s\r\n", port_buf[0] ? port_buf : "1883");
    printf("user: %s\r\n", user[0] ? user : "(anonymous)");
    printf("pass: %s\r\n", pass[0] ? "********" : "(empty)");
    printf("device_id: %s\r\n", device_id[0] ? device_id : "(mac)");
    printf("tls: %s\r\n", tls_buf);
    return 0;
}

static uint8_t mqtt_cli_set_host(uint8_t argc, char *argv[])
{
    if (argc < 1 || argv[0] == NULL) {
        printf("usage: mqtt set host <hostname>\r\n");
        return 1;
    }

    if (mqtt_cred_save_host(config_port_get(), argv[0]) != PORT_OK) {
        printf("invalid host\r\n");
        return 1;
    }

    printf("host saved\r\n");
    return 0;
}

static uint8_t mqtt_cli_set_port(uint8_t argc, char *argv[])
{
    unsigned long port_val;
    char *end;

    if (argc < 1 || argv[0] == NULL) {
        printf("usage: mqtt set port <port>\r\n");
        return 1;
    }

    port_val = strtoul(argv[0], &end, 10);
    if (end == argv[0] || *end != '\0' || port_val > 65535) {
        printf("invalid port\r\n");
        return 1;
    }

    if (mqtt_cred_save_port(config_port_get(), (uint16_t)port_val) != PORT_OK) {
        printf("invalid port\r\n");
        return 1;
    }

    printf("port saved\r\n");
    return 0;
}

static uint8_t mqtt_cli_set_user(uint8_t argc, char *argv[])
{
    const char *user = "";

    if (argc < 1 || argv[0] == NULL) {
        printf("usage: mqtt set user <username>\r\n");
        return 1;
    }

    user = argv[0];
    if (mqtt_cred_save_user(config_port_get(), user) != PORT_OK) {
        printf("nvdm write failed\r\n");
        return 1;
    }

    printf("user saved\r\n");
    return 0;
}

static uint8_t mqtt_cli_set_pass(uint8_t argc, char *argv[])
{
    const char *pass = "";

    if (argc < 1 || argv[0] == NULL) {
        printf("usage: mqtt set pass <password>\r\n");
        return 1;
    }

    pass = argv[0];
    if (mqtt_cred_save_pass(config_port_get(), pass) != PORT_OK) {
        printf("nvdm write failed\r\n");
        return 1;
    }

    printf("password saved\r\n");
    return 0;
}

static uint8_t mqtt_cli_set_device_id(uint8_t argc, char *argv[])
{
    const char *device_id = "";

    if (argc < 1 || argv[0] == NULL) {
        printf("usage: mqtt set device_id <id>\r\n");
        return 1;
    }

    device_id = argv[0];
    if (mqtt_cred_save_device_id(config_port_get(), device_id) != PORT_OK) {
        printf("invalid device_id\r\n");
        return 1;
    }

    printf("device_id saved\r\n");
    return 0;
}

static uint8_t mqtt_cli_set(uint8_t argc, char *argv[])
{
    if (argc < 1 || argv[0] == NULL) {
        printf("usage: mqtt set host|port|user|pass|device_id <value>\r\n");
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

    printf("usage: mqtt set host|port|user|pass|device_id <value>\r\n");
    return 1;
}

static uint8_t mqtt_cli_disconnect(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    mqtt_client_stop();
    printf("mqtt stopped\r\n");
    return 0;
}

static uint8_t mqtt_cli_connect(uint8_t argc, char *argv[])
{
    (void)argc;
    (void)argv;

    if (!mqtt_cred_is_stored(config_port_get())) {
        printf("set host first\r\n");
        return 1;
    }

    if (mqtt_client_connect_in_progress()) {
        printf("connect already in progress\r\n");
        return 1;
    }

    if (!mqtt_client_wifi_is_ready()) {
        printf("wifi not ready\r\n");
        return 1;
    }

    if (!mqtt_client_request_connect()) {
        printf("connect already in progress\r\n");
        return 1;
    }

    printf("connecting...\r\n");
    return 0;
}

cmd_t mqtt_cli_subcmds[] = {
    { "show",       "show broker settings", mqtt_cli_show,       NULL },
    { "set",        "set host|port|...",    mqtt_cli_set,        NULL },
    { "connect",    "connect to broker",    mqtt_cli_connect,    NULL },
    { "disconnect", "stop mqtt client",     mqtt_cli_disconnect, NULL },
    { NULL, NULL, NULL, NULL },
};
