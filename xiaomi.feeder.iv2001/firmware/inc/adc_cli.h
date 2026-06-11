/*
 * ADC bench CLI logic — spec/30-processes/uart-console.md
 */

#ifndef ADC_CLI_H
#define ADC_CLI_H

#include <stddef.h>
#include <stdint.h>

#include "adc_port.h"
#include "port_err.h"

void adc_cli_print_fail(port_err_t err);
void adc_cli_print_cal_fail(port_err_t err);

port_err_t adc_cli_run_motor_read(uint16_t *ma);
port_err_t adc_cli_run_battery_read(uint16_t *mv);
port_err_t adc_cli_run_cal_capture(uint16_t true_mv);
port_err_t adc_cli_run_cal_reset(void);
port_err_t adc_cli_run_cal_status(adc_cal_status_t *status);
void adc_cli_format_cal_status(const adc_cal_status_t *status, char *buf, size_t len);

#endif /* ADC_CLI_H */
