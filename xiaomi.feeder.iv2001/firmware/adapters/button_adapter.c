/*
 * GPIO4 EINT for AW9523B INT — spec/30-processes/wfci-bus-arbitration.md § Buttons and IRQ
 */

#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "hal_eint.h"
#include "hal_gpio.h"
#include "hal_pinmux_define.h"
#include "syslog.h"

#include "app_event.h"
#include "board_gpio_iv2001.h"
#include "button_adapter.h"
#include "gpio_expander_port.h"

log_create_module(button, PRINT_LEVEL_INFO);

extern bool app_event_post_from_isr(const app_event_t *ev, BaseType_t *wake);

#define AW9523B_INT_GPIO       HAL_GPIO_4
#define AW9523B_INT_EINT       HAL_EINT_NUMBER_4

static void button_adapter_eint_cb(void *user_data)
{
    app_event_t ev;
    BaseType_t wake = pdFALSE;

    (void)user_data;

    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_BUTTON_IRQ;
    ev.u.button_irq.now_ms =
        (uint32_t)(xTaskGetTickCountFromISR() * portTICK_PERIOD_MS);
    (void)app_event_post_from_isr(&ev, &wake);
    portYIELD_FROM_ISR(wake);
}

port_err_t button_adapter_start(void)
{
    hal_eint_config_t eint_cfg;
    port_err_t err;

    if (gpio_expander_port_get()->set_int_mask != NULL) {
        err = gpio_expander_port_get()->set_int_mask(BOARD_GPIO_AW9523_INT_MASK_P0,
                                                     BOARD_GPIO_AW9523_INT_MASK_P1);
        if (err != PORT_OK) {
            LOG_E(button, "IRQ mask write failed");
            return err;
        }
    }

    hal_gpio_init(AW9523B_INT_GPIO);
    hal_pinmux_set_function(AW9523B_INT_GPIO, HAL_GPIO_4_EINT4);

    hal_eint_mask(AW9523B_INT_EINT);
    eint_cfg.trigger_mode = HAL_EINT_EDGE_FALLING;
    eint_cfg.debounce_time = 0u;
    if (hal_eint_init(AW9523B_INT_EINT, &eint_cfg) != HAL_EINT_STATUS_OK) {
        LOG_E(button, "EINT init failed");
        return PORT_ERR_IO;
    }
    if (hal_eint_register_callback(AW9523B_INT_EINT,
                                   button_adapter_eint_cb,
                                   NULL) != HAL_EINT_STATUS_OK) {
        LOG_E(button, "EINT callback failed");
        return PORT_ERR_IO;
    }
    hal_eint_unmask(AW9523B_INT_EINT);

    LOG_I(button, "AW9523B IRQ armed on GPIO4");
    return PORT_OK;
}
