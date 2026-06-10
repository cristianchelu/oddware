/* Tests: spec/10-hardware/components/weigh-assp-cs1270.md */

#include "unity.h"

#include "cs1270.h"

static port_err_t s_exchange_err;
static uint8_t s_rsp[CS1270_FRAME_LEN];
static bool s_has_rsp;

static void set_rsp(uint8_t cmd3, uint8_t cmd2, uint8_t cmd1)
{
    s_rsp[0] = 0xB2u;
    s_rsp[1] = 0xA5u;
    s_rsp[2] = cmd3;
    s_rsp[3] = cmd2;
    s_rsp[4] = cmd1;
    s_rsp[5] = cs1270_checksum_rsp(cmd3, cmd2, cmd1);
    s_has_rsp = true;
    s_exchange_err = PORT_OK;
}

static port_err_t fake_exchange(const uint8_t tx[CS1270_FRAME_LEN],
                                uint8_t rx[CS1270_FRAME_LEN],
                                uint32_t timeout_ms)
{
    (void)tx;
    (void)timeout_ms;

    if (s_exchange_err != PORT_OK || !s_has_rsp) {
        return s_exchange_err != PORT_OK ? s_exchange_err : PORT_ERR_IO;
    }

    for (uint8_t i = 0u; i < CS1270_FRAME_LEN; i++) {
        rx[i] = s_rsp[i];
    }
    return PORT_OK;
}

static cs1270_uart_ops_t s_ops = {
    .exchange = fake_exchange,
    .delay_ms = NULL,
};

void test_cs1270_checksum_host_query(void)
{
    TEST_ASSERT_EQUAL_HEX8(0x2Cu, cs1270_checksum_host(0xCAu, 0xC2u, 0xEEu));
}

void test_cs1270_encode_cmd_query(void)
{
    uint8_t frame[CS1270_FRAME_LEN];

    cs1270_encode_cmd(frame, 0xCAu, 0xC2u, 0xEEu);
    TEST_ASSERT_EQUAL_HEX8(0xA1u, frame[0]);
    TEST_ASSERT_EQUAL_HEX8(0x5Au, frame[1]);
    TEST_ASSERT_EQUAL_HEX8(0xCAu, frame[2]);
    TEST_ASSERT_EQUAL_HEX8(0x2Cu, frame[5]);
}

void test_cs1270_parse_weight_positive(void)
{
    int32_t grams;

    set_rsp(0x00u, 0x0Cu, 0xE4u);
    TEST_ASSERT_TRUE(cs1270_verify_rsp(s_rsp));
    TEST_ASSERT_EQUAL(CS1270_STATUS_WEIGHT, cs1270_parse_response(s_rsp, &grams));
    TEST_ASSERT_EQUAL(3300, grams);
}

void test_cs1270_parse_weight_negative(void)
{
    int32_t grams;

    set_rsp(0x01u, 0x00u, 0x04u);
    TEST_ASSERT_EQUAL(CS1270_STATUS_WEIGHT, cs1270_parse_response(s_rsp, &grams));
    TEST_ASSERT_EQUAL(-4, grams);
}

void test_cs1270_parse_boot_warming(void)
{
    set_rsp(0x0Fu, 0xFFu, 0x88u);
    TEST_ASSERT_EQUAL(CS1270_STATUS_BOOT_WARMING,
                      cs1270_parse_response(s_rsp, NULL));
}

void test_cs1270_query_returns_weight(void)
{
    cs1270_status_t st;
    int32_t grams;

    set_rsp(0x00u, 0x00u, 0x64u);
    TEST_ASSERT_EQUAL(PORT_OK, cs1270_query(&s_ops, &grams, &st));
    TEST_ASSERT_EQUAL(CS1270_STATUS_WEIGHT, st);
    TEST_ASSERT_EQUAL(100, grams);
}
