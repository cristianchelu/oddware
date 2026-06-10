/*
 * GPIO4 EINT for AW9523B INT — spec/30-processes/button-handling.md § IRQ dispatch
 */

#include <stdbool.h>
#include <string.h>

#include "FreeRTOS.h"
#include "task.h"

#include "hal_eint.h"
#include "hal_gpio.h"
#include "hal_pinmux_define.h"
#include "syslog.h"

#include "app_event.h"
#include "aw9523_irq_adapter.h"
#include "board_gpio_iv2001.h"
#include "gpio_expander_port.h"

log_create_module(aw9523_irq, PRINT_LEVEL_INFO);

extern bool app_event_post_from_isr(const app_event_t *ev, BaseType_t *wake);

#define AW9523B_INT_GPIO  HAL_GPIO_4
#define AW9523B_INT_EINT    HAL_EINT_NUMBER_4

static struct {
    TaskHandle_t task;
    uint32_t notify_bits;
    bool armed;
} s_motor_notify;

static void aw9523_irq_adapter_eint_cb(void *user_data)
{
    app_event_t ev;
    BaseType_t wake = pdFALSE;

    (void)user_data;

    memset(&ev, 0, sizeof(ev));
    ev.type = EVT_BUTTON_IRQ;
    ev.u.button_irq.now_ms =
        (uint32_t)(xTaskGetTickCountFromISR() * portTICK_PERIOD_MS);
    (void)app_event_post_from_isr(&ev, &wake);

    if (s_motor_notify.armed && s_motor_notify.task != NULL) {
        (void)xTaskNotifyFromISR(s_motor_notify.task,
                                 s_motor_notify.notify_bits,
                                 eSetBits,
                                 &wake);
    }

    portYIELD_FROM_ISR(wake);
}

void aw9523_irq_adapter_register_motor_notify(TaskHandle_t task, uint32_t notify_bits)
{
    s_motor_notify.task = task;
    s_motor_notify.notify_bits = notify_bits;
    s_motor_notify.armed = (task != NULL);
}

void aw9523_irq_adapter_unregister_motor_notify(void)
{
    s_motor_notify.armed = false;
    s_motor_notify.task = NULL;
    s_motor_notify.notify_bits = 0u;
}

port_err_t aw9523_irq_adapter_start(void)
{
    hal_eint_config_t eint_cfg;
    port_err_t err;

    aw9523_irq_adapter_unregister_motor_notify();

    if (gpio_expander_port_get()->set_int_mask != NULL) {
        err = gpio_expander_port_get()->set_int_mask(BOARD_GPIO_AW9523_INT_MASK_P0,
                                                     BOARD_GPIO_AW9523_INT_MASK_P1);
        if (err != PORT_OK) {
            LOG_E(aw9523_irq, "IRQ mask write failed");
            return err;
        }
    }

    hal_gpio_init(AW9523B_INT_GPIO);
    hal_pinmux_set_function(AW9523B_INT_GPIO, HAL_GPIO_4_EINT4);

    hal_eint_mask(AW9523B_INT_EINT);
    eint_cfg.trigger_mode = HAL_EINT_EDGE_FALLING;
    eint_cfg.debounce_time = 0u;
    if (hal_eint_init(AW9523B_INT_EINT, &eint_cfg) != HAL_EINT_STATUS_OK) {
        LOG_E(aw9523_irq, "EINT init failed");
        return PORT_ERR_IO;
    }
    if (hal_eint_register_callback(AW9523B_INT_EINT,
                                   aw9523_irq_adapter_eint_cb,
                                   NULL) != HAL_EINT_STATUS_OK) {
        LOG_E(aw9523_irq, "EINT callback failed");
        return PORT_ERR_IO;
    }
    hal_eint_unmask(AW9523B_INT_EINT);

    LOG_I(aw9523_irq, "AW9523B IRQ armed on GPIO4");
    return PORT_OK;
}
