/*
 * AW9523B button IRQ — forwards to aw9523_irq_adapter.
 */

#include "aw9523_irq_adapter.h"
#include "button_adapter.h"

port_err_t button_adapter_start(void)
{
    return aw9523_irq_adapter_start();
}
