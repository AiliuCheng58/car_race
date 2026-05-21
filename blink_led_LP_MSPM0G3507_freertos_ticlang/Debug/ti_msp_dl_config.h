/*
 * Copyright (c) 2023, Texas Instruments Incorporated - http://www.ti.com
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ============ ti_msp_dl_config.h =============
 *  Configured MSPM0 DriverLib module declarations
 *
 *  DO NOT EDIT - This file is generated for the MSPM0G350X
 *  by the SysConfig tool.
 */
#ifndef ti_msp_dl_config_h
#define ti_msp_dl_config_h

#define CONFIG_MSPM0G350X
#define CONFIG_MSPM0G3507

#if defined(__ti_version__) || defined(__TI_COMPILER_VERSION__)
#define SYSCONFIG_WEAK __attribute__((weak))
#elif defined(__IAR_SYSTEMS_ICC__)
#define SYSCONFIG_WEAK __weak
#elif defined(__GNUC__)
#define SYSCONFIG_WEAK __attribute__((weak))
#endif

#include <ti/devices/msp/msp.h>
#include <ti/driverlib/driverlib.h>
#include <ti/driverlib/m0p/dl_core.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 *  ======== SYSCFG_DL_init ========
 *  Perform all required MSP DL initialization
 *
 *  This function should be called once at a point before any use of
 *  MSP DL.
 */


/* clang-format off */

#define POWER_STARTUP_DELAY                                                (16)



#define CPUCLK_FREQ                                                     32000000



/* Defines for TIMER_0 */
#define TIMER_0_INST                                                     (TIMA0)
#define TIMER_0_INST_IRQHandler                                 TIMA0_IRQHandler
#define TIMER_0_INST_INT_IRQN                                   (TIMA0_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                         (39999U)
#define TIMER_0_INST_PUB_0_CH                                                (1)
#define TIMER_0_INST_PUB_1_CH                                                (2)




/* Defines for I2C_1 */
#define I2C_1_INST                                                          I2C1
#define I2C_1_INST_IRQHandler                                    I2C1_IRQHandler
#define I2C_1_INST_INT_IRQN                                        I2C1_INT_IRQn
#define I2C_1_BUS_SPEED_HZ                                                100000
#define GPIO_I2C_1_SDA_PORT                                                GPIOA
#define GPIO_I2C_1_SDA_PIN                                         DL_GPIO_PIN_3
#define GPIO_I2C_1_IOMUX_SDA                                      (IOMUX_PINCM8)
#define GPIO_I2C_1_IOMUX_SDA_FUNC                       IOMUX_PINCM8_PF_I2C1_SDA
#define GPIO_I2C_1_SCL_PORT                                                GPIOA
#define GPIO_I2C_1_SCL_PIN                                         DL_GPIO_PIN_4
#define GPIO_I2C_1_IOMUX_SCL                                      (IOMUX_PINCM9)
#define GPIO_I2C_1_IOMUX_SCL_FUNC                       IOMUX_PINCM9_PF_I2C1_SCL


/* Defines for UART_0 */
#define UART_0_INST                                                        UART0
#define UART_0_INST_FREQUENCY                                           32000000
#define UART_0_INST_IRQHandler                                  UART0_IRQHandler
#define UART_0_INST_INT_IRQN                                      UART0_INT_IRQn
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_28
#define GPIO_UART_0_IOMUX_TX                                      (IOMUX_PINCM3)
#define GPIO_UART_0_IOMUX_TX_FUNC                       IOMUX_PINCM3_PF_UART0_TX
#define UART_0_BAUD_RATE                                                (115200)
#define UART_0_IBRD_32_MHZ_115200_BAUD                                      (17)
#define UART_0_FBRD_32_MHZ_115200_BAUD                                      (23)





/* Defines for ADC12_1 */
#define ADC12_1_INST                                                        ADC1
#define ADC12_1_INST_IRQHandler                                  ADC1_IRQHandler
#define ADC12_1_INST_INT_IRQN                                    (ADC1_INT_IRQn)
#define ADC12_1_ADCMEM_s8                                     DL_ADC12_MEM_IDX_0
#define ADC12_1_ADCMEM_s8_REF                    DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC12_1_ADCMEM_s8_REF_VOLTAGE_V                                      3.3
#define ADC12_1_ADCMEM_s7                                     DL_ADC12_MEM_IDX_1
#define ADC12_1_ADCMEM_s7_REF                    DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC12_1_ADCMEM_s7_REF_VOLTAGE_V                                      3.3
#define ADC12_1_ADCMEM_s6                                     DL_ADC12_MEM_IDX_2
#define ADC12_1_ADCMEM_s6_REF                    DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC12_1_ADCMEM_s6_REF_VOLTAGE_V                                      3.3
#define ADC12_1_ADCMEM_s5                                     DL_ADC12_MEM_IDX_3
#define ADC12_1_ADCMEM_s5_REF                    DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC12_1_ADCMEM_s5_REF_VOLTAGE_V                                      3.3
#define ADC12_1_INST_SUB_CH                                                  (2)
#define GPIO_ADC12_1_C5_PORT                                               GPIOB
#define GPIO_ADC12_1_C5_PIN                                       DL_GPIO_PIN_18
#define GPIO_ADC12_1_IOMUX_C5                                    (IOMUX_PINCM44)
#define GPIO_ADC12_1_IOMUX_C5_FUNC                (IOMUX_PINCM44_PF_UNCONNECTED)
#define GPIO_ADC12_1_C6_PORT                                               GPIOB
#define GPIO_ADC12_1_C6_PIN                                       DL_GPIO_PIN_19
#define GPIO_ADC12_1_IOMUX_C6                                    (IOMUX_PINCM45)
#define GPIO_ADC12_1_IOMUX_C6_FUNC                (IOMUX_PINCM45_PF_UNCONNECTED)
#define GPIO_ADC12_1_C7_PORT                                               GPIOA
#define GPIO_ADC12_1_C7_PIN                                       DL_GPIO_PIN_21
#define GPIO_ADC12_1_IOMUX_C7                                    (IOMUX_PINCM46)
#define GPIO_ADC12_1_IOMUX_C7_FUNC                (IOMUX_PINCM46_PF_UNCONNECTED)
#define GPIO_ADC12_1_C8_PORT                                               GPIOA
#define GPIO_ADC12_1_C8_PIN                                       DL_GPIO_PIN_22
#define GPIO_ADC12_1_IOMUX_C8                                    (IOMUX_PINCM47)
#define GPIO_ADC12_1_IOMUX_C8_FUNC                (IOMUX_PINCM47_PF_UNCONNECTED)

/* Defines for ADC12_0 */
#define ADC12_0_INST                                                        ADC0
#define ADC12_0_INST_IRQHandler                                  ADC0_IRQHandler
#define ADC12_0_INST_INT_IRQN                                    (ADC0_INT_IRQn)
#define ADC12_0_ADCMEM_s1                                     DL_ADC12_MEM_IDX_0
#define ADC12_0_ADCMEM_s1_REF                    DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC12_0_ADCMEM_s1_REF_VOLTAGE_V                                      3.3
#define ADC12_0_ADCMEM_s2                                     DL_ADC12_MEM_IDX_1
#define ADC12_0_ADCMEM_s2_REF                    DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC12_0_ADCMEM_s2_REF_VOLTAGE_V                                      3.3
#define ADC12_0_ADCMEM_s3                                     DL_ADC12_MEM_IDX_2
#define ADC12_0_ADCMEM_s3_REF                    DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC12_0_ADCMEM_s3_REF_VOLTAGE_V                                      3.3
#define ADC12_0_ADCMEM_s4                                     DL_ADC12_MEM_IDX_3
#define ADC12_0_ADCMEM_s4_REF                    DL_ADC12_REFERENCE_VOLTAGE_VDDA
#define ADC12_0_ADCMEM_s4_REF_VOLTAGE_V                                      3.3
#define ADC12_0_INST_SUB_CH                                                  (1)
#define GPIO_ADC12_0_C0_PORT                                               GPIOA
#define GPIO_ADC12_0_C0_PIN                                       DL_GPIO_PIN_27
#define GPIO_ADC12_0_IOMUX_C0                                    (IOMUX_PINCM60)
#define GPIO_ADC12_0_IOMUX_C0_FUNC                (IOMUX_PINCM60_PF_UNCONNECTED)
#define GPIO_ADC12_0_C1_PORT                                               GPIOA
#define GPIO_ADC12_0_C1_PIN                                       DL_GPIO_PIN_26
#define GPIO_ADC12_0_IOMUX_C1                                    (IOMUX_PINCM59)
#define GPIO_ADC12_0_IOMUX_C1_FUNC                (IOMUX_PINCM59_PF_UNCONNECTED)
#define GPIO_ADC12_0_C2_PORT                                               GPIOA
#define GPIO_ADC12_0_C2_PIN                                       DL_GPIO_PIN_25
#define GPIO_ADC12_0_IOMUX_C2                                    (IOMUX_PINCM55)
#define GPIO_ADC12_0_IOMUX_C2_FUNC                (IOMUX_PINCM55_PF_UNCONNECTED)
#define GPIO_ADC12_0_C3_PORT                                               GPIOA
#define GPIO_ADC12_0_C3_PIN                                       DL_GPIO_PIN_24
#define GPIO_ADC12_0_IOMUX_C3                                    (IOMUX_PINCM54)
#define GPIO_ADC12_0_IOMUX_C3_FUNC                (IOMUX_PINCM54_PF_UNCONNECTED)



/* Defines for s1: GPIOA.7 with pinCMx 14 on package pin 49 */
#define GPIO_LEDS_s1_PORT                                                (GPIOA)
#define GPIO_LEDS_s1_PIN                                         (DL_GPIO_PIN_7)
#define GPIO_LEDS_s1_IOMUX                                       (IOMUX_PINCM14)
/* Defines for s2: GPIOB.2 with pinCMx 15 on package pin 50 */
#define GPIO_LEDS_s2_PORT                                                (GPIOB)
#define GPIO_LEDS_s2_PIN                                         (DL_GPIO_PIN_2)
#define GPIO_LEDS_s2_IOMUX                                       (IOMUX_PINCM15)
/* Defines for s3: GPIOB.3 with pinCMx 16 on package pin 51 */
#define GPIO_LEDS_s3_PORT                                                (GPIOB)
#define GPIO_LEDS_s3_PIN                                         (DL_GPIO_PIN_3)
#define GPIO_LEDS_s3_IOMUX                                       (IOMUX_PINCM16)
/* Defines for s4: GPIOA.8 with pinCMx 19 on package pin 54 */
#define GPIO_LEDS_s4_PORT                                                (GPIOA)
#define GPIO_LEDS_s4_PIN                                         (DL_GPIO_PIN_8)
#define GPIO_LEDS_s4_IOMUX                                       (IOMUX_PINCM19)
/* Defines for s5: GPIOB.6 with pinCMx 23 on package pin 58 */
#define GPIO_LEDS_s5_PORT                                                (GPIOB)
#define GPIO_LEDS_s5_PIN                                         (DL_GPIO_PIN_6)
#define GPIO_LEDS_s5_IOMUX                                       (IOMUX_PINCM23)
/* Defines for s6: GPIOA.11 with pinCMx 22 on package pin 57 */
#define GPIO_LEDS_s6_PORT                                                (GPIOA)
#define GPIO_LEDS_s6_PIN                                        (DL_GPIO_PIN_11)
#define GPIO_LEDS_s6_IOMUX                                       (IOMUX_PINCM22)
/* Defines for s7: GPIOA.10 with pinCMx 21 on package pin 56 */
#define GPIO_LEDS_s7_PORT                                                (GPIOA)
#define GPIO_LEDS_s7_PIN                                        (DL_GPIO_PIN_10)
#define GPIO_LEDS_s7_IOMUX                                       (IOMUX_PINCM21)
/* Defines for s8: GPIOA.9 with pinCMx 20 on package pin 55 */
#define GPIO_LEDS_s8_PORT                                                (GPIOA)
#define GPIO_LEDS_s8_PIN                                         (DL_GPIO_PIN_9)
#define GPIO_LEDS_s8_IOMUX                                       (IOMUX_PINCM20)
/* Defines for RUN_LED: GPIOA.17 with pinCMx 39 on package pin 10 */
#define GPIO_LEDS_RUN_LED_PORT                                           (GPIOA)
#define GPIO_LEDS_RUN_LED_PIN                                   (DL_GPIO_PIN_17)
#define GPIO_LEDS_RUN_LED_IOMUX                                  (IOMUX_PINCM39)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_TIMER_0_init(void);
void SYSCFG_DL_I2C_1_init(void);
void SYSCFG_DL_UART_0_init(void);
void SYSCFG_DL_ADC12_1_init(void);
void SYSCFG_DL_ADC12_0_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
