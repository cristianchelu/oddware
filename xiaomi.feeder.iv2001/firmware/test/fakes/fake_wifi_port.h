#ifndef FAKE_WIFI_PORT_H
#define FAKE_WIFI_PORT_H

#include <stdbool.h>
#include <stdint.h>

#include "port_err.h"
#include "wifi_port.h"

typedef enum {
    FAKE_WIFI_OP_NONE = 0,
    FAKE_WIFI_OP_DISCONNECT,
    FAKE_WIFI_OP_SET_CREDENTIALS,
    FAKE_WIFI_OP_RADIO_UP,
    FAKE_WIFI_OP_ARM_CONNECT,
    FAKE_WIFI_OP_CONNECT,
    FAKE_WIFI_OP_WAIT_READY,
} fake_wifi_op_t;

#define FAKE_WIFI_OP_LOG_MAX 16u

typedef struct {
    uint32_t disconnect_calls;
    uint32_t set_credentials_calls;
    uint32_t radio_up_calls;
    uint32_t arm_connect_calls;
    uint32_t connect_calls;
    uint32_t wait_ready_calls;
    fake_wifi_op_t op_log[FAKE_WIFI_OP_LOG_MAX];
    uint32_t op_log_len;
    bool radio_on;
    bool connected;
    bool has_ip;
} fake_wifi_port_state_t;

void fake_wifi_port_reset(void);
const fake_wifi_port_state_t *fake_wifi_port_state(void);
const wifi_port_t *fake_wifi_port_get(void);
void fake_wifi_port_set_sta_up(bool connected, bool has_ip);
void fake_wifi_port_set_wait_ready_result(port_err_t result);

#endif /* FAKE_WIFI_PORT_H */
