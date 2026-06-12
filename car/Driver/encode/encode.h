#ifndef __ENCODE_H_
#define __ENCODE_H_ // 防止 encode.h 被重复包含

#include <stdint.h>

/*
 * encode 模块只负责“读取编码器”：
 * 1. 读取 E1A/E1B、E2A/E2B 对应的 GPIO
 * 2. 在 GPIO 中断里累计编码器总计数
 * 3. 提供累计计数快照，给 PID、里程计等模块各自计算增量
 */

/*
 * 编码器引脚由 car.syscfg 配置：
 * 左轮 A/B 相：PA8 / PA7
 * 右轮 A/B 相：PB7 / PB10
 */

/* 正转计数为负时，把对应方向系数改成 -1。 */
#define ENCODER_LEFT_DIR         (-1) // 左轮计数方向系数：实测前进时为负，所以取反
#define ENCODER_RIGHT_DIR        (1) // 右轮计数方向系数：车轮向前转时计数为负就改成 -1

/*
 * 一圈对应的编码器计数。
 * 注意这里要按实际电机校准：霍尔线数、减速比、是否四倍频都会影响这个值。
 */
#define ENCODER_LEFT_COUNTS_PER_REV   (1458.6f) // 左轮实测每转一圈的编码器计数
#define ENCODER_RIGHT_COUNTS_PER_REV  (1457.8f) // 右轮实测每转一圈的编码器计数
/* PID 定时器当前是 10 ms 取一次编码器增量。 */
#define ENCODER_SAMPLE_PERIOD_S  (0.01f) // 编码器采样周期，单位秒，要和 TIMER_0 周期一致

typedef struct {
    int32_t left; // 左轮编码器计数
    int32_t right; // 右轮编码器计数
} EncoderSample;

void encoder_init(void); // 初始化编码器状态并打开 GPIO 中断
EncoderSample encoder_get_total(void); // 读取左右轮累计计数快照，不清零
float encoder_left_count_to_rpm(int32_t count); // 左轮计数换算 RPM，使用左轮实测每圈计数
float encoder_right_count_to_rpm(int32_t count); // 右轮计数换算 RPM，使用右轮实测每圈计数

#endif
