/*
 * AW9523B micro-session helpers — spec/30-processes/wfci-bus-arbitration.md
 */

#ifndef GPIO_EXPANDER_LOAN_H
#define GPIO_EXPANDER_LOAN_H

#include <stdbool.h>

#include "port_err.h"

port_err_t gpio_expander_loan_begin(void);
port_err_t gpio_expander_loan_try_begin(void);
void gpio_expander_loan_end(void);
bool gpio_expander_loan_is_held(void);

#endif /* GPIO_EXPANDER_LOAN_H */
