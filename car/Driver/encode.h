#ifndef __ENCODE_H_
#define __ENCODE_H_

#include <stdint.h>
#include "ti_msp_dl_config.h"

/*
 * encode 模块只负责“读取编码器”：
 * 1. 读取 E1A/E1B、E2A/E2B 对应的 GPIO
 * 2. 在 GPIO 中断里累计编码器增量
 * 3. 把一个采样周期内的计数换算成 RPM
 */

/*
 * 编码器引脚由 car.syscfg 配置：
 * 左轮 A/B 相：PA6 / PA7
 * 右轮 A/B 相：PB9 / PB10
 */
#define ENCODER_LEFT_A_PORT      ENCODER_LEFT_PORT
#define ENCODER_LEFT_B_PORT      ENCODER_LEFT_PORT
#define ENCODER_RIGHT_A_PORT     ENCODER_RIGHT_PORT
#define ENCODER_RIGHT_B_PORT     ENCODER_RIGHT_PORT

#define ENCODER_LEFT_A_PIN       ENCODER_LEFT_LEFT_A_PIN
#define ENCODER_LEFT_B_PIN       ENCODER_LEFT_LEFT_B_PIN
#define ENCODER_RIGHT_A_PIN      ENCODER_RIGHT_RIGHT_A_PIN
#define ENCODER_RIGHT_B_PIN      ENCODER_RIGHT_RIGHT_B_PIN

#define ENCODER_LEFT_PIN_MASK    (ENCODER_LEFT_A_PIN | ENCODER_LEFT_B_PIN)
#define ENCODER_RIGHT_PIN_MASK   (ENCODER_RIGHT_A_PIN | ENCODER_RIGHT_B_PIN)

/* 正转计数为负时，把对应方向系数改成 -1。 */
#define ENCODER_LEFT_DIR         (1)
#define ENCODER_RIGHT_DIR        (1)

/*
 * 一圈对应的编码器计数。
 * 注意这里要按实际电机校准：霍尔线数、减速比、是否四倍频都会影响这个值。
 */
#define ENCODER_COUNTS_PER_REV   (500.0f)

/* PID 定时器当前是 10 ms 取一次编码器增量。 */
#define ENCODER_SAMPLE_PERIOD_S  (0.01f)

typedef struct {
    /* 一个采样周期内累计到的左右轮编码器增量。 */
    int32_t left;
    int32_t right;
} EncoderSample;

void encoder_init(void);
void encoder_clear(void);
int32_t encoder_get_left_count(void);
int32_t encoder_get_right_count(void);
EncoderSample encoder_get_and_clear(void);
float encoder_count_to_rpm(int32_t count);

#endif
