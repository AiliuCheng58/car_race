#include <stdint.h>
#include <stdbool.h>

#include "FreeRTOS.h"
#include "task.h"

#include "ti_msp_dl_config.h"

static void MainTask(void *pvParameters);

void app_main(void)
{
    xTaskCreate(
        MainTask,
        "Main",
        configMINIMAL_STACK_SIZE,
        NULL,
        tskIDLE_PRIORITY + 1,
        NULL
    );

    vTaskStartScheduler();

    while (1) {
    }
}

static void MainTask(void *pvParameters)
{
    (void) pvParameters;

    while (1) {
        

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}