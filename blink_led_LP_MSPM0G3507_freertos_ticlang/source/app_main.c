#include "app_main.h"
#include "head.h"

void app_main(void)
{
    BaseType_t ret;

    //xTaskCreate(INIT, "INIT", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);

    ret = xTaskCreate(LED, "LED", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);
    configASSERT(ret == pdPASS);

    //ret = xTaskCreate(ICM, "ICM", configMINIMAL_STACK_SIZE * 10, NULL, tskIDLE_PRIORITY + 2, NULL);
    //configASSERT(ret == pdPASS);

    ret = xTaskCreate(UART, "UART", configMINIMAL_STACK_SIZE * 5, NULL, tskIDLE_PRIORITY + 1, NULL);
    configASSERT(ret == pdPASS);

    vTaskStartScheduler();

    while(1){

    }
}
