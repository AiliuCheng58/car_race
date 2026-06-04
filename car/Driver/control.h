#ifndef __CONTROL_H_
#define __CONTROL_H_ // 防止 control.h 被重复包含

#include <stdint.h>

#define BASE_RPM            (50.0f)  // 基础前进速度，偏差为 0 时左右轮都跑这个 RPM
#define LINE_FOLLOW_KP      (7.0f)   // 循迹 P 系数，偏差越大，左右轮速度差越大
#define LINE_FOLLOW_KD      (2.0f)   // 循迹 D 系数，偏差变化越快，提前加一点修正
#define TURN_RPM            (35.0f)  // 循迹修正量限幅，避免一偏就原地打转
#define LOOP_MS             (5U)     // 主循环刷新周期，单位 ms
#define TIMEOUT_MS          (500U)   // 超过这个时间没有新循迹帧就停车
#define TELEMETRY_PERIOD_MS (200U)   // 遥测显示刷新周期，过快会占用较多任务时间

typedef struct {
    int32_t left_count; // 左轮最近一个 10ms PID 周期内的编码器计数
    int32_t right_count; // 右轮最近一个 10ms PID 周期内的编码器计数
    float left_rpm; // 左轮当前转速，单位 RPM
    float right_rpm; // 右轮当前转速，单位 RPM
    float left_pwm; // 左轮当前 PID 输出，带方向符号
    float right_pwm; // 右轮当前 PID 输出，带方向符号
    uint8_t track_raw; // 八路循迹原始值，bit0 在最左，bit7 在最右
    int8_t track_error; // 循迹偏差，左负右正
    uint8_t track_valid; // 当前是否有可用循迹偏差
} ControlTelemetry;

void control_init(void); // 初始化左右轮速度 PID，默认目标速度为 0
void control_get_telemetry(ControlTelemetry *data); // 获取遥测显示使用的控制状态快照

#endif
