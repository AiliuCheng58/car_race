#include "control.h"

#include "encode.h"
#include "motor.h"
#include "pid.h"
#include "ti_msp_dl_config.h"

#include <stddef.h>
#include <stdint.h>
#include "ti/driverlib/dl_timer.h"

PID pid_left;                                   // 左轮速度 PID 控制器
PID pid_right;                                  // 右轮速度 PID 控制器
static volatile ControlTelemetry telemetry;     // 串口监视任务读取的控制状态快照

void control_init(void)
{
    pid_config(&pid_left, SPEED_MODE, 0.0f, MOTOR_PWM_MAX / 2,
                PID_DEFAULT_KP, PID_DEFAULT_KI, PID_DEFAULT_KD,
                PID_CONTROL_PERIOD_S);                                  // 左轮速度环，输出限幅先压到 PWM 最大值的一半

    pid_config(&pid_right, SPEED_MODE, 0.0f, MOTOR_PWM_MAX / 2,
                PID_DEFAULT_KP, PID_DEFAULT_KI, PID_DEFAULT_KD,
                PID_CONTROL_PERIOD_S);                                  // 右轮速度环，参数先和左轮保持一致
}

void control_get_telemetry(ControlTelemetry *data)
{
    uint32_t primask;                                                       // 保存进入函数前的中断状态

    if (data == NULL) {                                                     // 空指针保护
        return;
    }

    primask = __get_PRIMASK();                                              // 保存原中断状态
    __disable_irq();                                                         // 复制快照时避免 10ms PID 中断改写数据
    *data = telemetry;                                                       // 一次复制完整状态快照
    __set_PRIMASK(primask);                                                  // 恢复原中断状态
}

void TIMER_0_INST_IRQHandler(void)
{
    EncoderSample sample = encoder_get_and_clear();                             // 读取并清零本周期编码器增量

    float left_cur = encoder_left_count_to_rpm(sample.left);                    // 左轮当前速度，单位 RPM
    float right_cur = encoder_right_count_to_rpm(sample.right);                 // 右轮当前速度，单位 RPM

    float left_output = pid_cal(&pid_left, left_cur);                           // 左轮速度 PID 输出 PWM
    motor_left_set(left_output);                                                // 输出到 TB6612 左路

    float right_output = pid_cal(&pid_right, right_cur);                         // 右轮速度 PID 输出 PWM
    motor_right_set(right_output);                                               // 输出到 TB6612 右路

    telemetry.left_count = sample.left;                                           // 保存左轮编码器计数，给串口监视任务打印
    telemetry.right_count = sample.right;                                         // 保存右轮编码器计数
    telemetry.left_rpm = left_cur;                                                 // 保存左轮 RPM
    telemetry.right_rpm = right_cur;                                               // 保存右轮 RPM
    telemetry.left_pwm = left_output;                                              // 保存左轮 PID 输出 PWM
    telemetry.right_pwm = right_output;                                            // 保存右轮 PID 输出 PWM

    DL_Timer_clearInterruptStatus(TIMER_0_INST, DL_TIMERA_INTERRUPT_ZERO_EVENT); // 清除 TIMER_0 中断标志
}
