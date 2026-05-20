#include "app_main.h"
#include "head.h"

void app_main(void)
{
    xTaskCreate(INIT, "INIT", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 3, NULL);

    xTaskCreate(LED, "LED", configMINIMAL_STACK_SIZE, NULL, tskIDLE_PRIORITY + 1, NULL);

    //xTaskCreate(ICM, "ICM", configMINIMAL_STACK_SIZE * 3, NULL, tskIDLE_PRIORITY + 2, NULL);

    //xTaskCreate(UART,  "UART",  configMINIMAL_STACK_SIZE * 3,  NULL,  tskIDLE_PRIORITY + 1,  NULL);

    vTaskStartScheduler();

    while(1){

    }
}
