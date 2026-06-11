/*
 * GPIO17 motor-load jam ISR — spec/30-processes/jam-detection.md
 */

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"

#include "adc_bus_adapter.h"
#include "adc_jam_isr_adapter.h"
#include "adc_limits.h"
#include "hal_adc.h"
#include "hal_eint.h"
#include "hal_gpio.h"
#include "hal_pinmux_define.h"
#include "motor_jam.h"

#define ADC_JAM_GPIO         HAL_GPIO_17
#define ADC_JAM_EINT         HAL_EINT_NUMBER_17
#define ADC_JAM_CHANNEL      HAL_ADC_CHANNEL_0
#define ADC_JAM_PINMUX_MODE  6u

static TaskHandle_t s_notify_task;
static uint32_t s_notify_bits;
static bool s_armed;

static uint16_t adc_jam_isr_raw_threshold(void)
{
    uint32_t raw = ((uint32_t)MOTOR_JAM_INSTANT_MA * (uint32_t)ADC_MAX_RAW) /
                   (uint32_t)ADC_REF_MV;

    if (raw > ADC_MAX_RAW) {
        raw = ADC_MAX_RAW;
    }

    return (uint16_t)raw;
}

static void adc_jam_isr_eint_cb(void *param)
{
    BaseType_t wake = pdFALSE;
    uint32_t data;
    uint16_t raw;
    uint16_t threshold = adc_jam_isr_raw_threshold();

    (void)param;

    if (hal_adc_get_data_polling(ADC_JAM_CHANNEL, &data) != HAL_ADC_STATUS_OK) {
        return;
    }

    raw = (uint16_t)(data & 0x0FFFu);
    if (raw <= threshold) {
        return;
    }

    if (s_notify_task != NULL) {
        (void)xTaskNotifyFromISR(s_notify_task, s_notify_bits, eSetBits, &wake);
        portYIELD_FROM_ISR(wake);
    }
}

void adc_jam_isr_adapter_start(TaskHandle_t task, uint32_t notify_bits)
{
    if (s_armed) {
        s_notify_task = task;
        s_notify_bits = notify_bits;
        return;
    }

    adc_bus_adapter_init();

    s_notify_task = task;
    s_notify_bits = notify_bits;

    {
        hal_eint_config_t cfg = {
            .debounce_time = 0,
            .trigger_mode = HAL_EINT_EDGE_RISING,
        };

        (void)hal_eint_init(ADC_JAM_EINT, &cfg);
        (void)hal_eint_register_callback(ADC_JAM_EINT, adc_jam_isr_eint_cb, NULL);
        hal_eint_unmask(ADC_JAM_EINT);
    }

    s_armed = true;
}

void adc_jam_isr_adapter_stop(void)
{
    if (!s_armed) {
        return;
    }

    hal_eint_mask(ADC_JAM_EINT);
    s_notify_task = NULL;
    s_notify_bits = 0u;
    s_armed = false;
}
