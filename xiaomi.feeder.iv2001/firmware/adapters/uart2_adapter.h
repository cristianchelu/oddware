/*
 * UART2 adapter for CS1270 — spec/10-hardware/components/weigh-assp-cs1270.md
 */

#ifndef UART2_ADAPTER_H
#define UART2_ADAPTER_H

#include <stdint.h>

#include "port_err.h"

void uart2_adapter_init(void);
void uart2_adapter_deinit(void);
port_err_t uart2_adapter_exchange(const uint8_t *tx, uint8_t *rx, uint32_t timeout_ms);

#endif /* UART2_ADAPTER_H */
