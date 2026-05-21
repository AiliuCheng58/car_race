#include "delay.h"

void delay_ms(uint32_t ms)
{
    if (ms == 0) 
        return;

    TickType_t ticks = pdMS_TO_TICKS(ms);
    
    if (ticks == 0) 
        ticks = 1;
    
    vTaskDelay(ticks);
}

