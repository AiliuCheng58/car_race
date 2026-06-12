#include "delay.h"

#include "ti_msp_dl_config.h"
#include "ti/driverlib/dl_common.h"

void delay_ms(uint32_t ms)
{
    delay_us(ms * 1000);
}

void delay_us(uint32_t us){
    while (us > 0U) {
        uint32_t step_us = (us > 100U) ? 100U : us;
        DL_Common_delayCycles((CPUCLK_FREQ / 1000000U) * step_us);
        us -= step_us; // 扣掉本次已经延时的毫秒数
    }
}