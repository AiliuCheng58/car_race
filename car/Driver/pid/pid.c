#include "pid.h"
#include "pid_internal.h"

#include <math.h>
#include <stddef.h>

static void pid_reset(PID *pid)
{
    if (pid == NULL)                                    // 空指针保护
        return;

    pid->integral = 0.0f;                               // 清积分累计
    pid->prev_err = 0.0f;                               // 清上一次误差
    pid->last_prev_err = 0.0f;                          // 清上上次误差
    pid->output = 0.0f;                                 // 清当前输出
}

static void pid_limit(PID *pid, uint8_t limit_output)
{
    if (pid == NULL)                                                        // 空指针保护
        return;

    if (fabsf(pid->ki) <= 0.000001f)                                        // ki 接近 0，说明积分项关闭
        pid->integral = 0.0f;                                               // 关闭积分时直接清掉累计值
    else {
        float integral_limit = (float) pid->integral_max / fabsf(pid->ki);  // 反推积分累计量上限

        if (pid->integral > integral_limit)                                 // 积分正向过大
            pid->integral = integral_limit;                                 // 压到正向上限
        else if (pid->integral < -integral_limit)                           // 积分负向过大
            pid->integral = -integral_limit;                                // 压到负向上限
    }

    if (limit_output == 0U)                                                 // 只需要处理积分限幅
        return;

    if (pid->output > (float) pid->pwm_max)                                 // 输出超过正向最大 PWM
        pid->output = (float) pid->pwm_max;                                 // 压到正向最大值
    else if (pid->output < -(float) pid->pwm_max)                           // 输出超过反向最大 PWM
        pid->output = -(float) pid->pwm_max;                                // 压到反向最大值

    if ((fabsf(pid->target) > PID_TARGET_DEADBAND_RPM) &&
        (fabsf(pid->output) > 0.000001f) &&
        (fabsf(pid->output) < PID_MIN_OUTPUT_PWM)) {                        // 非零目标但输出小于起转 PWM
        pid->output = (pid->output > 0.0f) ?
            PID_MIN_OUTPUT_PWM : -PID_MIN_OUTPUT_PWM;                       // 保持方向，只补到最小起转 PWM
    }
}

void pid_write_params(PID *pid, uint8_t has_kp, float kp,
    uint8_t has_ki, float ki, uint8_t has_kd, float kd)
{
    if (pid == NULL) { // 空指针保护
        return;
    }

    if (has_kp != 0U) { // 本次串口调参写了 kp
        pid->kp = kp; // 更新比例系数
    }
    if (has_ki != 0U) { // 本次串口调参写了 ki
        pid->ki = ki; // 更新积分系数
    }
    if (has_kd != 0U) { // 本次串口调参写了 kd
        pid->kd = kd; // 更新微分系数
    }

    pid_limit(pid, 0U); // 参数变化后整理积分，避免旧积分超限
}

void pid_config(PID *pid, enum pid_mode mode, float target, uint16_t pwm_max,
    float kp, float ki, float kd, float dt)
{
    if (pid == NULL)                                        // 空指针保护
        return;

    pid->target = target;                                   // 初始目标值，速度环里单位是 RPM
    pid->dt = (dt > 0.0f) ? dt : PID_CONTROL_PERIOD_S;      // dt 非法时回退到默认 10 ms
    pid->pwm_max = pwm_max;                                 // 输出限幅，避免超过 PWM 最大比较值
    pid->integral_max = pwm_max / 2U;                       // 积分项最多贡献一半 PWM，减少积分饱和
    pid->mode = ((mode == SPEED_MODE) || (mode == POSITION_MODE)) ?
        mode : POSITION_MODE;                               // 模式异常时回退到位置式

    pid_reset(pid);                                         // 先清历史状态，避免旧积分参与新参数
    pid_write_params(pid, 1U, kp, 1U, ki, 1U, kd);          // 初始化时三个参数都写入
}

void pid_set_target(PID *pid, float target)
{
    if (pid == NULL)                                 // 空指针保护
        return;

    if ((pid->target > 0.0f && target < 0.0f) ||
        (pid->target < 0.0f && target > 0.0f))
        pid_reset(pid);                              // 目标速度换向时清历史误差，避免旧积分继续推电机

    pid->target = target;                            // 保存新的目标值
}

float pid_cal(PID *pid, float cur)
{
    if (pid == NULL) { // 空指针保护
        return 0.0f;
    }

    if (fabsf(pid->target) <= PID_TARGET_DEADBAND_RPM) { // 目标速度接近 0
        pid_reset(pid); // 停车时清掉历史误差和积分
        return 0.0f; // 直接输出 0，让电机停止
    }

    float err = pid->target - cur; // 当前误差 = 目标值 - 当前值
    float delta_err = err - pid->prev_err; // 本次误差变化量
    float dt = (pid->dt > 0.0f) ? pid->dt : PID_CONTROL_PERIOD_S; // 使用配置周期，异常时回退

    switch (pid->mode) {
        case POSITION_MODE:
            pid->integral += err * dt; // 位置式 PID 累加积分
            pid_limit(pid, 0U); // 先限制积分，防止积分饱和
            pid->output = pid->kp * err // P：当前误差越大，输出越大
                + pid->ki * pid->integral // I：长期误差补偿
                + pid->kd * (delta_err / dt); // D：误差变化速度，用来抑制过冲
            break;

        case SPEED_MODE:
            pid->output += pid->kp * delta_err // 增量式 P：看误差变化
                + pid->ki * err * dt // 增量式 I：当前误差积分到输出增量
                + pid->kd * ((err - 2.0f * pid->prev_err
                    + pid->last_prev_err) / dt); // 增量式 D：二阶差分
            break;

        default:
            pid_reset(pid); // 模式异常时清状态
            return 0.0f; // 不输出，避免电机乱动
    }

    pid_limit(pid, 1U); // 统一处理输出限幅和最小起转 PWM
    pid->last_prev_err = pid->prev_err; // 更新上上次误差
    pid->prev_err = err; // 更新上一次误差

    return pid->output; // 返回带正负号的 PWM 输出
}
