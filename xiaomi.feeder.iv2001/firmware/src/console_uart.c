/*
 * UART0 override poll — spec/30-processes/uart-console.md § UART local override
 */

#include "console_uart.h"

#ifdef HOST_TEST

static bool s_has_byte;
static uint8_t s_pending_byte;

bool console_uart_rx_pending(void)
{
    return s_has_byte;
}

void console_uart_consume_pending(void)
{
    s_has_byte = false;
}

void console_uart_test_reset(void)
{
    s_has_byte = false;
    s_pending_byte = 0;
}

void console_uart_test_inject(uint8_t byte)
{
    s_pending_byte = byte;
    s_has_byte = true;
}

#else

#include "hal_uart.h"

bool console_uart_rx_pending(void)
{
    return hal_uart_get_char_unblocking(HAL_UART_0) != 0xFFFFFFFFu;
}

void console_uart_consume_pending(void)
{
    if (hal_uart_get_char_unblocking(HAL_UART_0) == 0xFFFFFFFFu) {
        return;
    }
}

#endif /* HOST_TEST */
