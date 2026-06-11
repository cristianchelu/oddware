#ifndef CONSOLE_UART_H
#define CONSOLE_UART_H

#include <stdbool.h>
#include <stdint.h>

bool console_uart_rx_pending(void);
void console_uart_consume_pending(void);

#ifdef HOST_TEST
void console_uart_test_reset(void);
void console_uart_test_inject(uint8_t byte);
#endif

#endif /* CONSOLE_UART_H */
