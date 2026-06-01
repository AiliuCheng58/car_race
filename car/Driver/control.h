#ifndef __CONTROL_H_
#define __CONTROL_H_ // 防止 control.h 被重复包含

#include <stdint.h>

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
void control_line_follow_loop(void); // 普通黑线循迹主循环
void control_get_telemetry(ControlTelemetry *data); // 获取串口打印使用的控制状态快照

#endif
