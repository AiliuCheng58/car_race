#include "control.h"

#include "encode.h"
#include "motor.h"
#include "pid.h"
#include "track.h"
#include "ti_msp_dl_config.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stddef.h>
#include <stdint.h>
#include "ti/driverlib/dl_timer.h"

#define BASE_RPM        (50.0f)         // 基础前进速度，偏差为 0 时左右轮都跑这个 RPM
#define KP              (7.0f)          // 循迹 P 系数，偏差越大，左右轮速度差越大
#define KD              (2.0f)          // 循迹 D 系数，偏差变化越快，提前加一点修正
#define TURN_RPM        (35.0f)         // 循迹修正量限幅，避免一偏就原地打转
#define LOOP_MS         (5U)            // 主循环刷新周期，单位 ms
#define TIMEOUT_MS      (500U)          // 超过这个时间没有新循迹帧就停车

static PID pid_left;                    // 左轮速度 PID 控制器
static PID pid_right;                   // 右轮速度 PID 控制器
static volatile ControlTelemetry telemetry; // 串口监视任务读取的控制状态快照

static int8_t last_error = 0;                     // 上一次循迹偏差，给 D 项使用
static float last_turn = 0.0f;                    // 上一次循迹修正量，用来做简单平滑

static float control_calc_turn_rpm(int8_t error); // 按偏差计算左右轮差速修正量

static float control_limit_float(float value, float limit)
{
    if (value > limit)   // 大于正限幅
        return limit;    // 压到正限幅
    if (value < -limit)  // 小于负限幅
        return -limit;   // 压到负限幅

    return value;        // 没超过限幅就原样返回
}

void control_init(void)
{
    pid_config(&pid_left, SPEED_MODE, 0.0f, MOTOR_PWM_MAX / 2,
                PID_DEFAULT_KP, PID_DEFAULT_KI, PID_DEFAULT_KD,
                PID_CONTROL_PERIOD_S);                                  // 左轮速度环，输出限幅先压到 PWM 最大值的一半

    pid_config(&pid_right, SPEED_MODE, 0.0f, MOTOR_PWM_MAX / 2,
                PID_DEFAULT_KP, PID_DEFAULT_KI, PID_DEFAULT_KD,
                PID_CONTROL_PERIOD_S);                                  // 右轮速度环，参数先和左轮保持一致
}

static float control_calc_turn_rpm(int8_t error)
{
    float p = (float) error * KP;                                        // P 项：当前位置偏差带来的修正
    float d = (float) (error - last_error) * KD;                         // D 项：偏差变化带来的修正
    float turn = control_limit_float(p + d, TURN_RPM);                   // 合成转向修正并限幅

    turn = 0.75f * turn + 0.25f * last_turn;                             // 对转向输出做一点平滑
    last_error = error;                                                  // 保存本次偏差，供下一次 D 项使用
    last_turn = turn;                                                    // 保存本次输出，供下一次平滑使用

    return turn;                                                         // 正数表示黑线在右边，左轮加速、右轮减速
}

void control_line_follow_loop(void)
{
    uint32_t track_age_ms = TIMEOUT_MS;                                  // 上电还没收到循迹帧，先按超时处理
    TickType_t last_wake_time = xTaskGetTickCount();                      // 记录循迹任务的固定周期起点

    while (1) {
        if (Track_receive() != 0U) track_age_ms = 0U;                    // 收到新帧就清零超时计数
        else if (track_age_ms < TIMEOUT_MS) track_age_ms += LOOP_MS;     // 没收到新帧就累计等待时间

        TrackInfo info = Track_get_info();                               // 获取最新 raw 和循迹偏差
        telemetry.track_raw = info.raw;                                  // 保存八路循迹原始值，给串口监视任务打印
        telemetry.track_error = info.error;                              // 保存当前循迹偏差
        telemetry.track_valid = info.valid;                              // 保存当前循迹偏差是否可用

        if ((info.valid == 0U) || (track_age_ms >= TIMEOUT_MS)) {
            pid_set_target(&pid_left, 0.0f);                              // 没有可靠循迹数据时左轮停车
            pid_set_target(&pid_right, 0.0f);                             // 没有可靠循迹数据时右轮停车
            last_error = 0;                                               // 清掉循迹 D 项历史偏差，恢复后重新计算
            last_turn = 0.0f;                                             // 清掉转向平滑量，恢复后避免突然打一把
        } else {
            float turn = control_calc_turn_rpm(info.error);              // 计算转向差速
            pid_set_target(&pid_left,  BASE_RPM + turn);                  // 更新左轮 RPM
            pid_set_target(&pid_right, BASE_RPM - turn);                       // 更新右轮 RPM
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(LOOP_MS));         // 每 5ms 唤醒一次，不阻塞其他 FreeRTOS 任务
    }
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
