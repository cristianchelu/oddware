/*
 * ADC sense limits — spec/30-processes/battery-monitoring.md,
 * spec/10-hardware/components/analog-mux-nc7sb3157.md
 */

#ifndef ADC_LIMITS_H
#define ADC_LIMITS_H

#include <stdint.h>

#define ADC_REF_MV              2500u
#define ADC_MAX_RAW             4095u
#define ADC_MUX_SETTLE_MS       1u
#define ADC_BATTERY_SAMPLE_CNT  10u

#define ADC_CAL_RATIO_MIN_X1000  8000u
#define ADC_CAL_RATIO_MAX_X1000  15000u
#define ADC_CAL_TRUE_MV_MIN      3000u
#define ADC_CAL_TRUE_MV_MAX      8000u
#define ADC_CAL_PIN_MV_MIN       100u

#endif /* ADC_LIMITS_H */
