/*
 * Battery percentage from pack voltage — spec/30-processes/battery-monitoring.md
 */

#ifndef BATTERY_PCT_H
#define BATTERY_PCT_H

#include <stdint.h>

typedef enum {
    BATTERY_CHEM_AA_ALK_4S = 0,
    BATTERY_CHEM_COUNT
} battery_chemistry_t;

battery_chemistry_t battery_pct_default_chemistry(void);
uint8_t battery_pct_from_mv(uint16_t pack_mv, battery_chemistry_t chem);

#endif /* BATTERY_PCT_H */
