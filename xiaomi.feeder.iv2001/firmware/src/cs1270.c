/*
 * CS1270 UART protocol — spec/10-hardware/components/weigh-assp-cs1270.md
 */

#include <stddef.h>

#include "cs1270.h"

#define CS1270_PREAMBLE_HOST  0xA1u
#define CS1270_HEADER_HOST    0x5Au
#define CS1270_PREAMBLE_RSP   0xB2u
#define CS1270_HEADER_RSP     0xA5u

#define CS1270_CMD3_STATUS    0x0Fu
#define CS1270_CMD3_WEIGHT_POS 0x00u
#define CS1270_CMD3_WEIGHT_NEG 0x01u

static uint8_t checksum_sum(uint8_t header, uint8_t cmd3, uint8_t cmd2, uint8_t cmd1)
{
    uint16_t sum = (uint16_t)header + cmd3 + cmd2 + cmd1;

    return (uint8_t)(sum & 0xFFu);
}

uint8_t cs1270_checksum_host(uint8_t cmd3, uint8_t cmd2, uint8_t cmd1)
{
    return (uint8_t)(~checksum_sum(CS1270_HEADER_HOST, cmd3, cmd2, cmd1) + 1u);
}

uint8_t cs1270_checksum_rsp(uint8_t cmd3, uint8_t cmd2, uint8_t cmd1)
{
    return (uint8_t)(~checksum_sum(CS1270_HEADER_RSP, cmd3, cmd2, cmd1) + 1u);
}

void cs1270_encode_cmd(uint8_t frame[CS1270_FRAME_LEN],
                       uint8_t cmd3, uint8_t cmd2, uint8_t cmd1)
{
    frame[0] = CS1270_PREAMBLE_HOST;
    frame[1] = CS1270_HEADER_HOST;
    frame[2] = cmd3;
    frame[3] = cmd2;
    frame[4] = cmd1;
    frame[5] = cs1270_checksum_host(cmd3, cmd2, cmd1);
}

bool cs1270_verify_rsp(const uint8_t frame[CS1270_FRAME_LEN])
{
    if (frame[0] != CS1270_PREAMBLE_RSP || frame[1] != CS1270_HEADER_RSP) {
        return false;
    }

    return frame[5] == cs1270_checksum_rsp(frame[2], frame[3], frame[4]);
}

cs1270_status_t cs1270_parse_response(const uint8_t frame[CS1270_FRAME_LEN],
                                      int32_t *grams)
{
    if (!cs1270_verify_rsp(frame)) {
        return CS1270_STATUS_INVALID;
    }

    if (frame[2] == CS1270_CMD3_WEIGHT_POS || frame[2] == CS1270_CMD3_WEIGHT_NEG) {
        if (grams != NULL) {
            int32_t val = (int32_t)(((uint16_t)frame[3] << 8) | frame[4]);

            if (frame[2] == CS1270_CMD3_WEIGHT_NEG) {
                val = -val;
            }
            *grams = val;
        }
        return CS1270_STATUS_WEIGHT;
    }

    if (frame[2] != CS1270_CMD3_STATUS) {
        return CS1270_STATUS_INVALID;
    }

    if (frame[3] == 0xFFu && frame[4] == 0x88u) {
        return CS1270_STATUS_BOOT_WARMING;
    }
    if (frame[3] == 0xF6u && frame[4] == 0x6Fu) {
        return CS1270_STATUS_ZERO_IN_PROGRESS;
    }
    if (frame[3] == 0xCAu && frame[4] == 0x3Au) {
        return CS1270_STATUS_CAL_CAPTURING_ZERO;
    }
    if (frame[3] == 0xCAu && frame[4] == 0x2Bu) {
        return CS1270_STATUS_CAL_CAPTURING_SPAN1;
    }
    if (frame[3] == 0xCAu && frame[4] == 0x1Cu) {
        return CS1270_STATUS_CAL_CAPTURING_SPAN2;
    }
    if (frame[3] == 0xCAu && frame[4] == 0xFFu) {
        return CS1270_STATUS_CAL_SUCCESS;
    }
    if (frame[3] == 0xCAu && frame[4] == 0x24u) {
        return CS1270_STATUS_UNCALIBRATED;
    }

    return CS1270_STATUS_INVALID;
}

port_err_t cs1270_exchange_cmd(const cs1270_uart_ops_t *ops,
                               uint8_t cmd3, uint8_t cmd2, uint8_t cmd1,
                               uint8_t rsp[CS1270_FRAME_LEN])
{
    uint8_t tx[CS1270_FRAME_LEN];

    if (ops == NULL || ops->exchange == NULL || rsp == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    cs1270_encode_cmd(tx, cmd3, cmd2, cmd1);
    return ops->exchange(tx, rsp, CS1270_UART_TIMEOUT_MS);
}

port_err_t cs1270_query(const cs1270_uart_ops_t *ops,
                        int32_t *grams,
                        cs1270_status_t *status)
{
    uint8_t rsp[CS1270_FRAME_LEN];
    port_err_t err;
    cs1270_status_t st;

    if (ops == NULL || status == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = cs1270_exchange_cmd(ops, 0xCAu, 0xC2u, 0xEEu, rsp);
    if (err != PORT_OK) {
        return err;
    }

    st = cs1270_parse_response(rsp, grams);
    if (st == CS1270_STATUS_INVALID) {
        return PORT_ERR_IO;
    }

    *status = st;
    return PORT_OK;
}
