/*
 * WFCI bus loan hooks — implemented in patched SDK wfcm_spi.c
 */

#ifndef WFCM_BUS_LOAN_H
#define WFCM_BUS_LOAN_H

void wfcm_bus_sync_init(void);
bool wfcm_bus_try_loan_begin(void);
bool wfcm_bus_loan_begin(void);
void wfcm_bus_loan_end(void);

#endif /* WFCM_BUS_LOAN_H */
