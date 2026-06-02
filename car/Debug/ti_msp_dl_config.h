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



/* Defines for PWM_0 */
#define PWM_0_INST                                                         TIMA0
#define PWM_0_INST_IRQHandler                                   TIMA0_IRQHandler
#define PWM_0_INST_INT_IRQN                                     (TIMA0_INT_IRQn)
#define PWM_0_INST_CLK_FREQ                                              8000000
/* GPIO defines for channel 0 */
#define GPIO_PWM_0_C0_PORT                                                 GPIOB
#define GPIO_PWM_0_C0_PIN                                         DL_GPIO_PIN_14
#define GPIO_PWM_0_C0_IOMUX                                      (IOMUX_PINCM31)
#define GPIO_PWM_0_C0_IOMUX_FUNC                     IOMUX_PINCM31_PF_TIMA0_CCP0
#define GPIO_PWM_0_C0_IDX                                    DL_TIMER_CC_0_INDEX
/* GPIO defines for channel 1 */
#define GPIO_PWM_0_C1_PORT                                                 GPIOB
#define GPIO_PWM_0_C1_PIN                                         DL_GPIO_PIN_12
#define GPIO_PWM_0_C1_IOMUX                                      (IOMUX_PINCM29)
#define GPIO_PWM_0_C1_IOMUX_FUNC                     IOMUX_PINCM29_PF_TIMA0_CCP1
#define GPIO_PWM_0_C1_IDX                                    DL_TIMER_CC_1_INDEX



/* Defines for TIMER_0 */
#define TIMER_0_INST                                                     (TIMA1)
#define TIMER_0_INST_IRQHandler                                 TIMA1_IRQHandler
#define TIMER_0_INST_INT_IRQN                                   (TIMA1_INT_IRQn)
#define TIMER_0_INST_LOAD_VALUE                                           (155U)




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
#define GPIO_UART_0_RX_PORT                                                GPIOA
#define GPIO_UART_0_TX_PORT                                                GPIOA
#define GPIO_UART_0_RX_PIN                                        DL_GPIO_PIN_11
#define GPIO_UART_0_TX_PIN                                        DL_GPIO_PIN_10
#define GPIO_UART_0_IOMUX_RX                                     (IOMUX_PINCM22)
#define GPIO_UART_0_IOMUX_TX                                     (IOMUX_PINCM21)
#define GPIO_UART_0_IOMUX_RX_FUNC                      IOMUX_PINCM22_PF_UART0_RX
#define GPIO_UART_0_IOMUX_TX_FUNC                      IOMUX_PINCM21_PF_UART0_TX
#define UART_0_BAUD_RATE                                                (115200)
#define UART_0_IBRD_32_MHZ_115200_BAUD                                      (17)
#define UART_0_FBRD_32_MHZ_115200_BAUD                                      (23)





/* Port definition for Pin Group MOTOR_ENABLE */
#define MOTOR_ENABLE_PORT                                                (GPIOA)

/* Defines for STBY: GPIOA.23 with pinCMx 53 on package pin 24 */
#define MOTOR_ENABLE_STBY_PIN                                   (DL_GPIO_PIN_23)
#define MOTOR_ENABLE_STBY_IOMUX                                  (IOMUX_PINCM53)
/* Port definition for Pin Group LED */
#define LED_PORT                                                         (GPIOB)

/* Defines for PIN_0: GPIOB.22 with pinCMx 50 on package pin 21 */
#define LED_PIN_0_PIN                                           (DL_GPIO_PIN_22)
#define LED_PIN_0_IOMUX                                          (IOMUX_PINCM50)
/* Port definition for Pin Group H2024_SIGNAL */
#define H2024_SIGNAL_PORT                                                (GPIOB)

/* Defines for BUZZER: GPIOB.23 with pinCMx 51 on package pin 22 */
#define H2024_SIGNAL_BUZZER_PIN                                 (DL_GPIO_PIN_23)
#define H2024_SIGNAL_BUZZER_IOMUX                                (IOMUX_PINCM51)
/* Port definition for Pin Group MOTOR_LEFT */
#define MOTOR_LEFT_PORT                                                  (GPIOA)

/* Defines for LEFT_IN1: GPIOA.24 with pinCMx 54 on package pin 25 */
#define MOTOR_LEFT_LEFT_IN1_PIN                                 (DL_GPIO_PIN_24)
#define MOTOR_LEFT_LEFT_IN1_IOMUX                                (IOMUX_PINCM54)
/* Defines for LEFT_IN2: GPIOA.25 with pinCMx 55 on package pin 26 */
#define MOTOR_LEFT_LEFT_IN2_PIN                                 (DL_GPIO_PIN_25)
#define MOTOR_LEFT_LEFT_IN2_IOMUX                                (IOMUX_PINCM55)
/* Port definition for Pin Group MOTOR_RIGHT */
#define MOTOR_RIGHT_PORT                                                 (GPIOB)

/* Defines for RIGHT_IN1: GPIOB.25 with pinCMx 56 on package pin 27 */
#define MOTOR_RIGHT_RIGHT_IN1_PIN                               (DL_GPIO_PIN_25)
#define MOTOR_RIGHT_RIGHT_IN1_IOMUX                              (IOMUX_PINCM56)
/* Defines for RIGHT_IN2: GPIOB.26 with pinCMx 57 on package pin 28 */
#define MOTOR_RIGHT_RIGHT_IN2_PIN                               (DL_GPIO_PIN_26)
#define MOTOR_RIGHT_RIGHT_IN2_IOMUX                              (IOMUX_PINCM57)
/* Port definition for Pin Group ENCODER_LEFT */
#define ENCODER_LEFT_PORT                                                (GPIOA)

/* Defines for LEFT_A: GPIOA.8 with pinCMx 19 on package pin 54 */
// pins affected by this interrupt request:["LEFT_A","LEFT_B"]
#define ENCODER_LEFT_INT_IRQN                                   (GPIOA_INT_IRQn)
#define ENCODER_LEFT_INT_IIDX                   (DL_INTERRUPT_GROUP1_IIDX_GPIOA)
#define ENCODER_LEFT_LEFT_A_IIDX                             (DL_GPIO_IIDX_DIO8)
#define ENCODER_LEFT_LEFT_A_PIN                                  (DL_GPIO_PIN_8)
#define ENCODER_LEFT_LEFT_A_IOMUX                                (IOMUX_PINCM19)
/* Defines for LEFT_B: GPIOA.7 with pinCMx 14 on package pin 49 */
#define ENCODER_LEFT_LEFT_B_IIDX                             (DL_GPIO_IIDX_DIO7)
#define ENCODER_LEFT_LEFT_B_PIN                                  (DL_GPIO_PIN_7)
#define ENCODER_LEFT_LEFT_B_IOMUX                                (IOMUX_PINCM14)
/* Port definition for Pin Group ENCODER_RIGHT */
#define ENCODER_RIGHT_PORT                                               (GPIOB)

/* Defines for RIGHT_A: GPIOB.9 with pinCMx 26 on package pin 61 */
// pins affected by this interrupt request:["RIGHT_A","RIGHT_B"]
#define ENCODER_RIGHT_INT_IRQN                                  (GPIOB_INT_IRQn)
#define ENCODER_RIGHT_INT_IIDX                  (DL_INTERRUPT_GROUP1_IIDX_GPIOB)
#define ENCODER_RIGHT_RIGHT_A_IIDX                           (DL_GPIO_IIDX_DIO9)
#define ENCODER_RIGHT_RIGHT_A_PIN                                (DL_GPIO_PIN_9)
#define ENCODER_RIGHT_RIGHT_A_IOMUX                              (IOMUX_PINCM26)
/* Defines for RIGHT_B: GPIOB.10 with pinCMx 27 on package pin 62 */
#define ENCODER_RIGHT_RIGHT_B_IIDX                          (DL_GPIO_IIDX_DIO10)
#define ENCODER_RIGHT_RIGHT_B_PIN                               (DL_GPIO_PIN_10)
#define ENCODER_RIGHT_RIGHT_B_IOMUX                              (IOMUX_PINCM27)
/* Port definition for Pin Group H2024_BUTTONS */
#define H2024_BUTTONS_PORT                                               (GPIOB)

/* Defines for MODE: GPIOB.0 with pinCMx 12 on package pin 47 */
#define H2024_BUTTONS_MODE_PIN                                   (DL_GPIO_PIN_0)
#define H2024_BUTTONS_MODE_IOMUX                                 (IOMUX_PINCM12)
/* Defines for START: GPIOB.1 with pinCMx 13 on package pin 48 */
#define H2024_BUTTONS_START_PIN                                  (DL_GPIO_PIN_1)
#define H2024_BUTTONS_START_IOMUX                                (IOMUX_PINCM13)


/* clang-format on */

void SYSCFG_DL_init(void);
void SYSCFG_DL_initPower(void);
void SYSCFG_DL_GPIO_init(void);
void SYSCFG_DL_SYSCTL_init(void);
void SYSCFG_DL_PWM_0_init(void);
void SYSCFG_DL_TIMER_0_init(void);
void SYSCFG_DL_I2C_1_init(void);
void SYSCFG_DL_UART_0_init(void);


bool SYSCFG_DL_saveConfiguration(void);
bool SYSCFG_DL_restoreConfiguration(void);

#ifdef __cplusplus
}
#endif

#endif /* ti_msp_dl_config_h */
