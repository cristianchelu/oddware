/*
 * ADC port adapter — spec/40-architecture/ports.md
 */

#include "FreeRTOS.h"
#include "semphr.h"
#include "task.h"

#include "adc_bus_adapter.h"
#include "adc_driver.h"
#include "adc_port.h"
#include "gpio_expander_port.h"
#include "wfci_bus_port.h"

#define ADC_ADAPTER_MUTEX_WAIT_MS  5000u

static adc_driver_state_t s_state;
static bool s_adc_ready;
static SemaphoreHandle_t s_adc_mutex;

static void adc_hal_delay_ms(uint32_t ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
}

static port_err_t adc_adapter_read_raw(uint16_t *raw)
{
    const wfci_bus_port_t *bus = wfci_bus_port_get();
    port_err_t err;

    if (raw == NULL || bus == NULL || bus->acquire == NULL ||
        bus->release == NULL) {
        return PORT_ERR_INVALID_ARG;
    }

    err = bus->acquire(WFCI_BUS_PROFILE_ADC,
                       WFCI_BUS_PRIORITY_HIGH,
                       ADC_ADAPTER_MUTEX_WAIT_MS);
    if (err != PORT_OK) {
        return err;
    }

    err = adc_bus_adapter_read_raw(raw);
    bus->release(WFCI_BUS_PROFILE_ADC);
    return err;
}

static void adc_adapter_ensure_init(void)
{
    adc_hw_t hw;

    if (s_adc_ready) {
        return;
    }

    hw.expander = gpio_expander_port_get();
    hw.read_raw = adc_adapter_read_raw;
    hw.delay_ms = adc_hal_delay_ms;
    adc_driver_init(&s_state, &hw);
    s_adc_ready = true;
}

static port_err_t adc_mutex_ensure(void)
{
    if (s_adc_mutex == NULL) {
        s_adc_mutex = xSemaphoreCreateMutex();
        if (s_adc_mutex == NULL) {
            return PORT_ERR_IO;
        }
    }

    return PORT_OK;
}

static port_err_t adc_port_read_locked(uint16_t *mv, bool battery)
{
    port_err_t err;

    err = adc_mutex_ensure();
    if (err != PORT_OK) {
        return err;
    }

    if (xSemaphoreTake(s_adc_mutex, pdMS_TO_TICKS(ADC_ADAPTER_MUTEX_WAIT_MS)) != pdPASS) {
        return PORT_ERR_BUSY;
    }

    adc_adapter_ensure_init();
    if (battery) {
        err = adc_driver_read_battery_mv(&s_state, mv);
    } else {
        err = adc_driver_read_motor_load_mv(&s_state, mv);
    }

    (void)xSemaphoreGive(s_adc_mutex);
    return err;
}

static port_err_t adc_port_read_motor_load_mv(uint16_t *mv)
{
    return adc_port_read_locked(mv, false);
}

static port_err_t adc_port_read_battery_mv(uint16_t *mv)
{
    return adc_port_read_locked(mv, true);
}

static const adc_port_t s_adc_port = {
    .read_motor_load_mv = adc_port_read_motor_load_mv,
    .read_battery_mv = adc_port_read_battery_mv,
};

const adc_port_t *adc_port_get(void)
{
    return &s_adc_port;
}
