/*
 * CS1270 UART protocol — spec/10-hardware/components/weigh-assp-cs1270.md
 */

#ifndef CS1270_H
#define CS1270_H

#include <stdbool.h>
#include <stdint.h>

#include "port_err.h"

#define CS1270_FRAME_LEN           6u
#define CS1270_BOOT_MS             1100u
#define CS1270_POLL_MS             100u
#define CS1270_UART_TIMEOUT_MS     200u
#define CS1270_UART_RETRY_MS       100u
#define CS1270_POWER_RETRIES       3u
#define CS1270_WARM_POLL_MAX       15u

typedef enum {
    CS1270_STATUS_WEIGHT = 0,
    CS1270_STATUS_BOOT_WARMING,
    CS1270_STATUS_ZERO_IN_PROGRESS,
    CS1270_STATUS_CAL_CAPTURING_ZERO,
    CS1270_STATUS_CAL_CAPTURING_SPAN1,
    CS1270_STATUS_CAL_CAPTURING_SPAN2,
    CS1270_STATUS_CAL_SUCCESS,
    CS1270_STATUS_UNCALIBRATED,
    CS1270_STATUS_INVALID,
} cs1270_status_t;

typedef struct cs1270_uart_ops {
    port_err_t (*exchange)(const uint8_t tx[CS1270_FRAME_LEN],
                           uint8_t rx[CS1270_FRAME_LEN],
                           uint32_t timeout_ms);
    void (*delay_ms)(uint32_t ms);
} cs1270_uart_ops_t;

uint8_t cs1270_checksum_host(uint8_t cmd3, uint8_t cmd2, uint8_t cmd1);
uint8_t cs1270_checksum_rsp(uint8_t cmd3, uint8_t cmd2, uint8_t cmd1);
void cs1270_encode_cmd(uint8_t frame[CS1270_FRAME_LEN],
                       uint8_t cmd3, uint8_t cmd2, uint8_t cmd1);
bool cs1270_verify_rsp(const uint8_t frame[CS1270_FRAME_LEN]);
cs1270_status_t cs1270_parse_response(const uint8_t frame[CS1270_FRAME_LEN],
                                      int32_t *grams);

port_err_t cs1270_exchange_cmd(const cs1270_uart_ops_t *ops,
                               uint8_t cmd3, uint8_t cmd2, uint8_t cmd1,
                               uint8_t rsp[CS1270_FRAME_LEN]);
port_err_t cs1270_query(const cs1270_uart_ops_t *ops,
                        int32_t *grams,
                        cs1270_status_t *status);

#endif /* CS1270_H */
