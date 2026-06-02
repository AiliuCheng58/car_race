#include "control.h"

#include "encode.h"
#include "motor.h"
#include "pid.h"
#include "ti_msp_dl_config.h"

#include "ti/driverlib/dl_timer.h"

#include <stddef.h>
#include <stdint.h>

#define WHEEL_DIAMETER_CM (6.5f) // 小车轮胎直径，实车测量后可在这里校准
#define PI_VALUE          (3.1415926f) // 计算轮胎周长使用的圆周率

static PID pid_left; // 左轮速度 PID 控制器
static PID pid_right; // 右轮速度 PID 控制器
static volatile ControlTelemetry telemetry; // 串口监视任务读取的底盘状态快照
static volatile uint32_t distance_left_count = 0U; // 当前路段左轮累计编码器计数
static volatile uint32_t distance_right_count = 0U; // 当前路段右轮累计编码器计数

static uint32_t control_abs_count(int32_t count)
{
    return (count >= 0) ? (uint32_t) count : (uint32_t) (-count); // 里程只累计绝对值，不区分方向
}

void control_init(void)
{
    pid_config(&pid_left, SPEED_MODE, 0.0f, MOTOR_PWM_MAX / 2,
        PID_DEFAULT_KP, PID_DEFAULT_KI, PID_DEFAULT_KD,
        PID_CONTROL_PERIOD_S); // 左轮速度环，输出限幅为 PWM 最大值的一半

    pid_config(&pid_right, SPEED_MODE, 0.0f, MOTOR_PWM_MAX / 2,
        PID_DEFAULT_KP, PID_DEFAULT_KI, PID_DEFAULT_KD,
        PID_CONTROL_PERIOD_S); // 右轮速度环，参数先和左轮保持一致

    control_reset_distance(); // 上电时路段里程从 0 开始
    control_stop(); // 上电后等待按键启动，不让电机自动转动
}

void control_set_target(float left_rpm, float right_rpm)
{
    uint32_t primask = __get_PRIMASK(); // 保存进入函数前的中断状态

    __disable_irq(); // 避免 PID 中断只读到一侧的新目标
    pid_set_target(&pid_left, left_rpm); // 设置左轮目标速度
    pid_set_target(&pid_right, right_rpm); // 设置右轮目标速度
    telemetry.left_target_rpm = left_rpm; // 保存左轮目标速度，给串口打印
    telemetry.right_target_rpm = right_rpm; // 保存右轮目标速度，给串口打印
    __set_PRIMASK(primask); // 恢复进入函数前的中断状态
}

void control_stop(void)
{
    control_set_target(0.0f, 0.0f); // 两侧目标速度同时清零
}

void control_reset_distance(void)
{
    uint32_t primask = __get_PRIMASK(); // 保存进入函数前的中断状态

    __disable_irq(); // 避免 PID 中断正在累计里程
    distance_left_count = 0U; // 清左轮路段里程
    distance_right_count = 0U; // 清右轮路段里程
    __set_PRIMASK(primask); // 恢复进入函数前的中断状态
}

float control_get_distance_cm(void)
{
    uint32_t primask = __get_PRIMASK(); // 保存进入函数前的中断状态
    uint32_t left_count; // 保存左轮累计计数快照
    uint32_t right_count; // 保存右轮累计计数快照
    float left_cm; // 左轮行驶距离
    float right_cm; // 右轮行驶距离

    __disable_irq(); // 复制累计计数时避免 PID 中断改写
    left_count = distance_left_count; // 复制左轮累计值
    right_count = distance_right_count; // 复制右轮累计值
    __set_PRIMASK(primask); // 恢复进入函数前的中断状态

    left_cm = ((float) left_count / ENCODER_LEFT_COUNTS_PER_REV)
        * (WHEEL_DIAMETER_CM * PI_VALUE); // 左轮计数换算成厘米
    right_cm = ((float) right_count / ENCODER_RIGHT_COUNTS_PER_REV)
        * (WHEEL_DIAMETER_CM * PI_VALUE); // 右轮计数换算成厘米

    return (left_cm + right_cm) * 0.5f; // 左右轮取平均值，减少单轮误差
}

void control_get_telemetry(ControlTelemetry *data)
{
    uint32_t primask; // 保存进入函数前的中断状态

    if (data == NULL) {
        return; // 空指针保护
    }

    primask = __get_PRIMASK(); // 保存原中断状态
    __disable_irq(); // 复制快照时避免 10ms PID 中断改写数据
    *data = telemetry; // 一次复制完整状态快照
    __set_PRIMASK(primask); // 恢复原中断状态

    data->distance_cm = control_get_distance_cm(); // 里程换算放在任务中执行，不占用定时器中断时间
}

void TIMER_0_INST_IRQHandler(void)
{
    EncoderSample sample = encoder_get_and_clear(); // 读取并清零本周期编码器增量
    float left_cur = encoder_left_count_to_rpm(sample.left); // 左轮当前速度，单位 RPM
    float right_cur = encoder_right_count_to_rpm(sample.right); // 右轮当前速度，单位 RPM
    float left_output = pid_cal(&pid_left, left_cur); // 左轮速度 PID 输出 PWM
    float right_output = pid_cal(&pid_right, right_cur); // 右轮速度 PID 输出 PWM

    motor_left_set(left_output); // 左轮输出到 TB6612
    motor_right_set(right_output); // 右轮输出到 TB6612

    distance_left_count += control_abs_count(sample.left); // 累计左轮路段里程
    distance_right_count += control_abs_count(sample.right); // 累计右轮路段里程

    telemetry.left_count = sample.left; // 保存左轮最近 10ms 编码器计数
    telemetry.right_count = sample.right; // 保存右轮最近 10ms 编码器计数
    telemetry.left_rpm = left_cur; // 保存左轮 RPM
    telemetry.right_rpm = right_cur; // 保存右轮 RPM
    telemetry.left_pwm = left_output; // 保存左轮 PID 输出 PWM
    telemetry.right_pwm = right_output; // 保存右轮 PID 输出 PWM

    DL_Timer_clearInterruptStatus(TIMER_0_INST,
        DL_TIMERA_INTERRUPT_ZERO_EVENT); // 清除 TIMER_0 中断标志
}
