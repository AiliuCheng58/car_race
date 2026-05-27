#ifndef __MOTOR_H_
#define __MOTOR_H_

#include <stdint.h>
#include "ti_msp_dl_config.h"

/*
 * motor 模块只负责“输出到 TB6612”：
 * 1. STBY 使能
 * 2. AIN1/AIN2、BIN1/BIN2 控制方向
 * 3. PWMA/PWMB 控制占空比
 *
 * 编码器计数和速度换算放在 encode 模块，不放在这里。
 */

/* 当前 PWM_0 周期按 1000 使用，所以占空比比较值限制在 0~1000。 */
#define MOTOR_PWM_MAX              (1000U)

void motor_init(void);
void motor_left_set(float output, int8_t direction);
void motor_right_set(float output, int8_t direction);
void motor_stop(void);

#endif
