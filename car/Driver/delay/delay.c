#include "delay.h"

#include "ti_msp_dl_config.h"
#include "ti/driverlib/dl_common.h"

void delay_ms(uint32_t ms)
{
    while (ms > 0U) { // 还剩延时时间就继续分段等待
        uint32_t step_ms = (ms > 100U) ? 100U : ms; // 单次最多延时 100ms，避免 cycles 计算过大
        DL_Common_delayCycles((CPUCLK_FREQ / 1000U) * step_ms); // 按 CPU 主频换算成延时周期数
        ms -= step_ms; // 扣掉本次已经延时的毫秒数
    }
}
