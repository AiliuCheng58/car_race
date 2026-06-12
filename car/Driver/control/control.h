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
} ControlTelemetry;

void control_init(void); // 初始化左右轮速度 PID，默认目标速度为 0
void control_set_target_rpm(float left_rpm, float right_rpm); // 设置左右轮目标速度，单位 RPM
void control_stop(void); // 左右轮目标速度清零
void control_read_pid_uart(void); // 从 UART 解析 PID 调参命令并更新左右轮速度环
void control_get_telemetry(ControlTelemetry *data); // 获取遥测显示使用的控制状态快照

#endif
