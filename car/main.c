/* Standard includes. */
#include <stdio.h>

/* Kernel includes. */
#include "FreeRTOS.h"
#include "task.h"

/* TI includes */
#include "ti_msp_dl_config.h"

static void prvSetupHardware(void);

extern void main_blinky(void);

/*-----------------------------------------------------------*/

int main(void)
{
    /* Prepare the hardware to run this demo. */
    prvSetupHardware();

    main_blinky();

    return 0;
}
