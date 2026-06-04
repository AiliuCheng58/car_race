#include "button.h"
#include "2task.h"
#include "FreeRTOS.h"
#include "FreeRTOSConfig.h"
#include "ti_msp_dl_config.h"
#include "ti/driverlib/dl_gpio.h"

void Button_init(void)
{
    button_sem = xSemaphoreCreateBinary();
    configASSERT(button_sem != NULL);
    button_task = 1;

    DL_GPIO_clearInterruptStatus(KEY_PORT, KEY_PIN_21_PIN);
    NVIC_ClearPendingIRQ(GPIO_MULTIPLE_GPIOB_INT_IRQN);
}

uint8_t Button_is_pressed(void)
{
    return (DL_GPIO_readPins(KEY_PORT, KEY_PIN_21_PIN) & KEY_PIN_21_PIN) == 0U;
}

void Button_switch(void)
{
    button_task = (button_task + 1) % 4;
}
