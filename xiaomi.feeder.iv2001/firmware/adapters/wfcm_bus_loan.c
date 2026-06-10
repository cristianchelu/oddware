/*
 * WFCI bus loan primitives — spec/30-processes/wfci-bus-arbitration.md
 *
 * Mirrors wfcm_if_deinit / wfcm_if_reinit from SDK wfcm_spi.c so loans work
 * without compiling stub_conf into the petfeeder image.
 */

#include "FreeRTOS.h"
#include "semphr.h"

#include "hal_gpio.h"
#include "hal_spi_master.h"

#include "wfcm_bus_loan.h"

#define WFCM_SPI_PORT          HAL_SPI_MASTER_0
#define WFCM_SPI_FREQ_HZ       10000000u
#define WFCM_PIN_CS            HAL_GPIO_17
#define WFCM_PIN_CLK           HAL_GPIO_16
#define WFCM_PIN_SIO_0         HAL_GPIO_15
#define WFCM_PIN_SIO_1         HAL_GPIO_14
#define WFCM_PIN_FUNC_ALT      2

static SemaphoreHandle_t s_bus_mutex;
static volatile bool s_bus_loaned;

static void wfcm_bus_mutex_ensure(void)
{
    if (s_bus_mutex == NULL) {
        s_bus_mutex = xSemaphoreCreateRecursiveMutex();
    }
}

static void wfcm_spi_pins_deinit(void)
{
    (void)hal_spi_master_deinit(WFCM_SPI_PORT);
    hal_gpio_deinit(WFCM_PIN_CS);
    hal_gpio_deinit(WFCM_PIN_CLK);
    hal_gpio_deinit(WFCM_PIN_SIO_0);
    hal_gpio_deinit(WFCM_PIN_SIO_1);
}

static void wfcm_spi_pins_reinit(void)
{
    hal_spi_master_config_t cfg;
    hal_spi_master_advanced_config_t advanced;

    cfg.bit_order = HAL_SPI_MASTER_LSB_FIRST;
    cfg.clock_frequency = WFCM_SPI_FREQ_HZ;
    cfg.phase = HAL_SPI_MASTER_CLOCK_PHASE0;
    cfg.polarity = HAL_SPI_MASTER_CLOCK_POLARITY0;

    if (hal_spi_master_init(WFCM_SPI_PORT, &cfg) != HAL_SPI_MASTER_STATUS_OK) {
        return;
    }

    advanced.byte_order = HAL_SPI_MASTER_LITTLE_ENDIAN;
    advanced.chip_polarity = HAL_SPI_MASTER_CHIP_SELECT_LOW;
    advanced.get_tick = HAL_SPI_MASTER_GET_TICK_DELAY2;
    advanced.sample_select = HAL_SPI_MASTER_SAMPLE_POSITIVE;
    (void)hal_spi_master_set_advanced_config(WFCM_SPI_PORT, &advanced);

    hal_gpio_init(WFCM_PIN_CS);
    hal_gpio_init(WFCM_PIN_CLK);
    hal_gpio_init(WFCM_PIN_SIO_0);
    hal_gpio_init(WFCM_PIN_SIO_1);
    hal_pinmux_set_function(WFCM_PIN_CS, WFCM_PIN_FUNC_ALT);
    hal_pinmux_set_function(WFCM_PIN_CLK, WFCM_PIN_FUNC_ALT);
    hal_pinmux_set_function(WFCM_PIN_SIO_0, WFCM_PIN_FUNC_ALT);
    hal_pinmux_set_function(WFCM_PIN_SIO_1, WFCM_PIN_FUNC_ALT);
}

bool wfcm_bus_try_loan_begin(void)
{
    wfcm_bus_mutex_ensure();
    if (xSemaphoreTakeRecursive(s_bus_mutex, 0) != pdPASS) {
        return false;
    }
    if (s_bus_loaned) {
        (void)xSemaphoreGiveRecursive(s_bus_mutex);
        return false;
    }
    s_bus_loaned = true;
    wfcm_spi_pins_deinit();
    return true;
}

void wfcm_bus_loan_begin(void)
{
    wfcm_bus_mutex_ensure();
    (void)xSemaphoreTakeRecursive(s_bus_mutex, portMAX_DELAY);
    s_bus_loaned = true;
    wfcm_spi_pins_deinit();
}

void wfcm_bus_loan_end(void)
{
    wfcm_spi_pins_reinit();
    s_bus_loaned = false;
    (void)xSemaphoreGiveRecursive(s_bus_mutex);
}
