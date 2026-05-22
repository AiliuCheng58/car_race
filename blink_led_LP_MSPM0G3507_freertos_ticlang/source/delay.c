#include "delay.h"
#include "ti/driverlib/m0p/dl_core.h"

#define MS 32000
#define US 32

void delay_ms(uint32_t ms)
{
    if (ms == 0) 
        return;

    TickType_t ticks = pdMS_TO_TICKS(ms);
    
    if (ticks == 0) 
        ticks = 1;
    
    vTaskDelay(ticks);
}

void delay_us(uint32_t us){
    return delay_cycles(US *us);
}

void delay_MS(uint32_t ms){
    return delay_cycles(MS * ms);
}
