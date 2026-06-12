#ifndef __MOTOR_H_
#define __MOTOR_H_

#define MOTOR_PWM_MAX              (1000U) // PWM 比较值最大值，对应 100% 占空比
#define MOTOR_LEFT_FORWARD_SIGN    (1.0f) // 左轮前进方向系数：正指令反着转就改成 -1
#define MOTOR_RIGHT_FORWARD_SIGN   (1.0f) // 右轮前进方向系数：小车原地旋转时优先改这里

void motor_init(void); // 初始化 TB6612 使能脚、方向脚和 PWM 输出
void motor_left_set(float output); // 设置左轮输出，正负号决定方向，绝对值决定 PWM
void motor_right_set(float output); // 设置右轮输出，正负号决定方向，绝对值决定 PWM

#endif
