#ifndef __CONTROL_H_
#define __CONTROL_H_ // 防止 control.h 被重复包含

#include <stdint.h>

typedef struct {
    int32_t left_count; // 左轮最近一个 10ms PID 周期内的编码器计数
    int32_t right_count; // 右轮最近一个 10ms PID 周期内的编码器计数
    float left_rpm; // 左轮当前转速，单位 RPM
    float right_rpm; // 右轮当前转速，单位 RPM
    float left_target_rpm; // 左轮目标转速，单位 RPM
    float right_target_rpm; // 右轮目标转速，单位 RPM
    float left_pwm; // 左轮当前 PID 输出，带方向符号
    float right_pwm; // 右轮当前 PID 输出，带方向符号
    float distance_cm; // 当前路段已经行驶的平均距离，单位 cm
} ControlTelemetry;

void control_init(void); // 初始化左右轮速度 PID 和路段里程
void control_set_target(float left_rpm, float right_rpm); // 同时更新左右轮目标速度
void control_stop(void); // 左右轮目标速度清零
void control_reset_distance(void); // 开始新路段前清零累计里程
float control_get_distance_cm(void); // 获取当前路段平均行驶距离，单位 cm
void control_get_telemetry(ControlTelemetry *data); // 获取串口打印使用的底盘状态快照

#endif
