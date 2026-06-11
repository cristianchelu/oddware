/*
 * ADC bench CLI logic — spec/30-processes/uart-console.md
 */

#ifndef ADC_CLI_H
#define ADC_CLI_H

#include <stdint.h>

#include "port_err.h"

void adc_cli_print_fail(port_err_t err);

port_err_t adc_cli_run_motor_read(uint16_t *mv);

port_err_t adc_cli_run_battery_read(uint16_t *mv);

#endif /* ADC_CLI_H */
