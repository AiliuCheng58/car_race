#ifndef __PID_H_
#define __PID_H_

#include <stdint.h>

#define PID_DEFAULT_KP  (1.1f) // 默认比例系数，影响纠正力度
#define PID_DEFAULT_KI  (0.0f) // 默认先关闭积分，确认反馈方向正确后再逐步加
#define PID_DEFAULT_KD  (0.0f) // 默认微分系数，先关闭，避免编码器噪声放大
#define PID_TARGET_DEADBAND_RPM (1.0f) // 目标转速低于这个值时直接认为是停车
#define PID_CONTROL_PERIOD_S (0.01f) // PID 计算周期，单位秒，对应 TIMER_0 的 10 ms
#define PID_MIN_OUTPUT_PWM (120.0f) // 非零目标下的最小起转 PWM，低于这个值电机可能不动

enum pid_mode {
    SPEED_MODE,
    POSITION_MODE
};

typedef struct PID {
    float prev_err; // 上一次误差，速度式和位置式都会用到
    float last_prev_err; // 上上次误差，速度式 PID 的二阶差分要用
    float integral; // 积分累计值，位置式 PID 使用

    float kp; // 比例系数
    float ki; // 积分系数
    float kd; // 微分系数

    float target; // 目标值，速度环里单位是 RPM
    float dt; // PID 计算间隔，单位秒

    int16_t pwm_max; // 输出限幅，避免超过 PWM 最大比较值
    int16_t integral_max; // 积分限幅，防止积分饱和

    float output; // 当前 PID 输出

    enum pid_mode mode; // 当前 PID 模式：速度式或位置式
} PID;

void pid_config(PID *pid, enum pid_mode mode, float target, uint16_t pwm_max,
    float kp, float ki, float kd, float dt); // 配置 PID 初值、限幅和计算周期
void pid_set_target(PID *pid, float target); // 修改目标值，换向时会清历史误差
void pid_set(PID *left_pid, PID *right_pid); // 从串口解析 kp/ki/kd 命令并更新左右轮
float pid_cal(PID *pid, float cur); // 输入当前值，返回 PID 输出

#endif
