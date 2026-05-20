#include "Driver/icm20608.h"
#include "ti_msp_dl_config.h"
#include "source/app_main.h"

int main(void)
{
    SYSCFG_DL_init();
    NVIC_EnableIRQ(ADC12_0_INST_INT_IRQN);
    NVIC_EnableIRQ(ADC12_1_INST_INT_IRQN);

    DL_ADC12_enableConversions(ADC12_1_INST);
    DL_ADC12_enableConversions(ADC12_0_INST);
    DL_TimerA_startCounter(TIMER_0_INST);

    app_main();
    return 0;
}
