/*
 * UART2 adapter for CS1270 — spec/10-hardware/components/weigh-assp-cs1270.md
 */

#include "FreeRTOS.h"
#include "task.h"

#include "hal_gpt.h"
#include "hal_gpio.h"
#include "hal_pinmux_define.h"
#include "hal_uart.h"

#include "cs1270.h"
#include "uart2_adapter.h"

#define UART2_PORT      HAL_UART_2
#define UART2_RX_GPIO   HAL_GPIO_11
#define UART2_TX_GPIO   HAL_GPIO_12

static bool s_uart2_ready;
static bool s_had_exchange;

static void uart2_flush_rx(void)
{
    for (;;) {
        if (hal_uart_get_char_unblocking(UART2_PORT) == 0xFFFFFFFFu) {
            break;
        }
    }
}

static void uart2_ensure_spacing(void)
{
    if (s_had_exchange) {
        hal_gpt_delay_ms(CS1270_POLL_MS);
    }
}

static void uart2_pin_init(void)
{
    hal_gpio_init(UART2_RX_GPIO);
    hal_pinmux_set_function(UART2_RX_GPIO, HAL_GPIO_11_URXD2);
    hal_gpio_init(UART2_TX_GPIO);
    hal_pinmux_set_function(UART2_TX_GPIO, HAL_GPIO_12_UTXD2);
}

void uart2_adapter_init(void)
{
    hal_uart_config_t cfg;

    if (s_uart2_ready) {
        return;
    }

    hal_uart_deinit(UART2_PORT);
    uart2_pin_init();

    cfg.baudrate = HAL_UART_BAUDRATE_9600;
    cfg.parity = HAL_UART_PARITY_NONE;
    cfg.stop_bit = HAL_UART_STOP_BIT_1;
    cfg.word_length = HAL_UART_WORD_LENGTH_8;

    if (hal_uart_init(UART2_PORT, &cfg) != HAL_UART_STATUS_OK) {
        return;
    }

    s_uart2_ready = true;
}

void uart2_adapter_deinit(void)
{
    if (!s_uart2_ready) {
        return;
    }

    hal_uart_deinit(UART2_PORT);
    s_uart2_ready = false;
    s_had_exchange = false;
}

static bool uart2_read_byte(uint8_t *out, uint32_t timeout_ms)
{
    uint32_t elapsed = 0u;
    const uint32_t step_ms = 5u;

    while (elapsed < timeout_ms) {
        uint32_t value = hal_uart_get_char_unblocking(UART2_PORT);

        if (value != 0xFFFFFFFFu) {
            *out = (uint8_t)(value & 0xFFu);
            return true;
        }

        hal_gpt_delay_ms(step_ms);
        elapsed += step_ms;
    }

    return false;
}

static port_err_t uart2_recv_frame(uint8_t rx[CS1270_FRAME_LEN], uint32_t timeout_ms)
{
    uint32_t elapsed = 0u;
    const uint32_t step_ms = 5u;

    while (elapsed < timeout_ms) {
        uint8_t byte;

        if (!uart2_read_byte(&byte, timeout_ms - elapsed)) {
            return PORT_ERR_IO;
        }

        if (byte == 0xB2u) {
            rx[0] = byte;
            if (!uart2_read_byte(&rx[1], timeout_ms)) {
                return PORT_ERR_IO;
            }
            if (rx[1] == 0xA5u) {
                for (uint8_t i = 2u; i < CS1270_FRAME_LEN; i++) {
                    if (!uart2_read_byte(&rx[i], timeout_ms)) {
                        return PORT_ERR_IO;
                    }
                }
                return PORT_OK;
            }
        }

        hal_gpt_delay_ms(step_ms);
        elapsed += step_ms;
    }

    return PORT_ERR_IO;
}

port_err_t uart2_adapter_exchange(const uint8_t *tx, uint8_t *rx, uint32_t timeout_ms)
{
    if (!s_uart2_ready || tx == NULL || rx == NULL) {
        return PORT_ERR_IO;
    }

    uart2_ensure_spacing();
    uart2_flush_rx();

    if (hal_uart_send_polling(UART2_PORT, tx, CS1270_FRAME_LEN) != CS1270_FRAME_LEN) {
        return PORT_ERR_IO;
    }

    s_had_exchange = true;
    hal_gpt_delay_ms(CS1270_POLL_MS);
    return uart2_recv_frame(rx, timeout_ms);
}
