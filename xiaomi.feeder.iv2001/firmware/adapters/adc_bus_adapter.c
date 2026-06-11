/*
 * AUXADC0 bus adapter — spec/30-processes/wfci-bus-arbitration.md
 */

#include "hal_adc.h"
#include "hal_gpio.h"
#include "hal_pinmux_define.h"

#include "adc_bus_adapter.h"
#include "port_err.h"

#define ADC_GPIO         HAL_GPIO_17
#define ADC_CHANNEL      HAL_ADC_CHANNEL_0
#define ADC_PINMUX_MODE  6u

static bool s_adc_ready;

void adc_bus_adapter_deinit(void)
{
    if (!s_adc_ready) {
        return;
    }

    (void)hal_adc_deinit();
    s_adc_ready = false;
}

void adc_bus_adapter_init(void)
{
    if (s_adc_ready) {
        return;
    }

    hal_gpio_init(ADC_GPIO);
    hal_pinmux_set_function(ADC_GPIO, ADC_PINMUX_MODE);
    (void)hal_adc_init();
    s_adc_ready = true;
}

port_err_t adc_bus_adapter_read_raw(uint16_t *raw)
{
    uint32_t data;
    hal_adc_status_t status;

    if (raw == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    if (!s_adc_ready) {
        return PORT_ERR_IO;
    }

    status = hal_adc_get_data_polling(ADC_CHANNEL, &data);
    if (status != HAL_ADC_STATUS_OK) {
        return PORT_ERR_IO;
    }

    *raw = (uint16_t)(data & 0x0FFFu);
    return PORT_OK;
}
