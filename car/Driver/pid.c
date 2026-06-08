#include "pid.h"
#include "uart.h"
#include "ti_msp_dl_config.h"

#include <math.h>
#include <stddef.h>
#include <stdlib.h>

#define PID_UART_LINE_MAX_LEN (96U) // 单条串口调参文本的最大缓存长度

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
         else if (pid->integral < -integral_limit)                          // 积分负向过大
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

static void pid_write_params(PID *pid, uint8_t has_kp, float kp,
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

void pid_read(PID *left_pid, PID *right_pid)
{
    if ((left_pid == NULL) && (right_pid == NULL)) {
        return;
    }

    char line[PID_UART_LINE_MAX_LEN]; // 保存串口收到的一整行文本
    uint16_t line_end = 0U;           // 指向 CR 或 LF 的位置
    uint16_t copy_len;                // 实际复制到 line 的字符数
    uint16_t consume_len;             // 需要从 UART 缓存移除的字符数
    uint32_t primask;                 // 保存进入函数前的中断状态

    float kp = 0.0f;
    float ki = 0.0f;
    float kd = 0.0f;
    uint8_t has_kp = 0U;
    uint8_t has_ki = 0U;
    uint8_t has_kd = 0U;

    primask = __get_PRIMASK();
    __disable_irq();

    while (line_end < RX_index) {
        if ((UART_Data[line_end] == '\n') || (UART_Data[line_end] == '\r')) {
            break;
        }
        line_end++;
    }

    if (line_end >= RX_index) { // 还没有完整一行
        __set_PRIMASK(primask);
        return;
    }

    copy_len = line_end;
    if (copy_len >= sizeof(line)) {
        copy_len = sizeof(line) - 1U;
    }

    for (uint16_t i = 0U; i < copy_len; i++) {
        line[i] = (char) UART_Data[i];
    }
    line[copy_len] = '\0';

    consume_len = line_end + 1U;
    while ((consume_len < RX_index) &&
        ((UART_Data[consume_len] == '\n') || (UART_Data[consume_len] == '\r'))) {
        consume_len++;
    }

    __set_PRIMASK(primask);

    char *text = line;

    // 跳过行首可能混入的无关字节，找到 kp/ki/kd 的 k
    while (*text != '\0') {
        char ch = *text;
        if ((ch >= 'A') && (ch <= 'Z')) {
            ch = (char) (ch + ('a' - 'A'));
        }

        if (ch == 'k') {
            break;
        }

        text++;
    }

    // 这一行不是 PID 调参文本，不消费缓存，留给循迹帧解析
    if (*text == '\0') {
        return;
    }

    // 确认是 PID 调参文本后，移除这一整行
    UART_MoveBytes(consume_len);

    while (*text != '\0') {
        while ((*text == ' ') || (*text == '\t') || (*text == ',')) {
            text++;
        }

        if (*text == '\0') {
            break;
        }

        char k = text[0];
        char item = text[1];

        if ((k >= 'A') && (k <= 'Z')) {
            k = (char) (k + ('a' - 'A'));
        }
        if ((item >= 'A') && (item <= 'Z')) {
            item = (char) (item + ('a' - 'A'));
        }

        if ((k != 'k') || ((item != 'p') && (item != 'i') && (item != 'd'))) {
            UART_SendString("PID ERR: use kp=1.0, ki=0.1, kd=0.0\r\n");
            return;
        }

        text += 2;

        while ((*text == ' ') || (*text == '\t')) {
            text++;
        }

        if (*text != '=') {
            UART_SendString("PID ERR: use kp=1.0, ki=0.1, kd=0.0\r\n");
            return;
        }

        text++;

        while ((*text == ' ') || (*text == '\t')) {
            text++;
        }

        char *end = text;
        float value = strtof(text, &end);

        if (end == text) {
            UART_SendString("PID ERR: use kp=1.0, ki=0.1, kd=0.0\r\n");
            return;
        }

        if (item == 'p') {
            kp = value;
            has_kp = 1U;
        } else if (item == 'i') {
            ki = value;
            has_ki = 1U;
        } else {
            kd = value;
            has_kd = 1U;
        }

        text = end;

        while ((*text == ' ') || (*text == '\t')) {
            text++;
        }

        if ((*text != ',') && (*text != '\0')) {
            UART_SendString("PID ERR: use kp=1.0, ki=0.1, kd=0.0\r\n");
            return;
        }
    }

    if ((has_kp == 0U) && (has_ki == 0U) && (has_kd == 0U)) {
        UART_SendString("PID ERR: use kp=1.0, ki=0.1, kd=0.0\r\n");
        return;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    pid_write_params(left_pid, has_kp, kp, has_ki, ki, has_kd, kd);
    pid_write_params(right_pid, has_kp, kp, has_ki, ki, has_kd, kd);
    __set_PRIMASK(primask);

    UART_SendString("PID OK\r\n");
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
