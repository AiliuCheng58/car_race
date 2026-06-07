#include "track.h"

#include "ti_msp_dl_config.h"
#include <sys/cdefs.h>

volatile uint8_t track = 0;

volatile uint16_t TRACK_THRESHOLD[8]={2800, 2700, 2800, 2700, 3100, 3000, 3500, 2800};

volatile uint16_t adc0[4]={0};
volatile uint16_t adc1[4]={0};

volatile bool ready_0=false;
volatile bool ready_1=false;

void setTrackBit(uint8_t bit, uint16_t adc, uint8_t i)
{
    if (adc > TRACK_THRESHOLD[i])
        track |= (1 << bit);
    else
        track &= ~(1 << bit);
}

void ADC12_0_INST_IRQHandler(void)
{
    switch (DL_ADC12_getPendingInterrupt(ADC12_0_INST))
    {
        case DL_ADC12_IIDX_MEM3_RESULT_LOADED:
            adc0[0] = DL_ADC12_getMemResult(ADC12_0_INST, ADC12_0_ADCMEM_s1);
            adc0[1] = DL_ADC12_getMemResult(ADC12_0_INST, ADC12_0_ADCMEM_s2);
            adc0[2] = DL_ADC12_getMemResult(ADC12_0_INST, ADC12_0_ADCMEM_s3);
            adc0[3] = DL_ADC12_getMemResult(ADC12_0_INST, ADC12_0_ADCMEM_s4);
            setTrackBit(0, adc0[0], 0);  // s1 最右
            setTrackBit(1, adc0[1], 1);  // s2
            setTrackBit(2, adc0[2], 2);  // s3
            setTrackBit(3, adc0[3], 3);  // s4
            
            ready_0 = true;
            DL_ADC12_enableConversions(ADC12_0_INST);
            break;

        default:
            break;
    }
}

void ADC12_1_INST_IRQHandler(void)
{
    switch (DL_ADC12_getPendingInterrupt(ADC12_1_INST))
    {
        case DL_ADC12_IIDX_MEM3_RESULT_LOADED:
            adc1[0] = DL_ADC12_getMemResult(ADC12_1_INST, ADC12_1_ADCMEM_s5);
            adc1[1] = DL_ADC12_getMemResult(ADC12_1_INST, ADC12_1_ADCMEM_s6);
            adc1[2] = DL_ADC12_getMemResult(ADC12_1_INST, ADC12_1_ADCMEM_s7);
            adc1[3] = DL_ADC12_getMemResult(ADC12_1_INST, ADC12_1_ADCMEM_s8);
            setTrackBit(7, adc1[0], 4);  // s5
            setTrackBit(6, adc1[1], 5);  // s6
            setTrackBit(5, adc1[2], 6);  // s7
            setTrackBit(4, adc1[3], 7);  // s8

            ready_1 = true;
            DL_ADC12_enableConversions(ADC12_1_INST);
            break;

        default:
            break;
    }
}