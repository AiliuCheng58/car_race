#ifndef __H2024_H_
#define __H2024_H_ // 防止 h2024.h 被重复包含

#include <stdint.h>

typedef enum {
    H2024_STATE_WAIT = 0U, // 待机：MODE 选择要求，START 启动
    H2024_STATE_STRAIGHT, // 无轨直线：使用 MPU6050 保持航向
    H2024_STATE_ARC, // 黑色半圆弧：使用八路灰度循迹
    H2024_STATE_FINISHED, // 当前任务已经完成
    H2024_STATE_ERROR // 传感器异常或弧线循迹数据超时
} H2024State;

typedef struct {
    uint8_t selected_task; // 当前选择的题目要求：1~4
    uint8_t state; // 当前 H2024State
    uint8_t segment; // 当前任务中的路段序号，从 0 开始
    uint8_t lap; // 当前圈数，从 1 开始
    uint8_t imu_ready; // MPU6050 是否初始化并完成零偏校准
    uint8_t track_raw; // 八路循迹模块最新 raw
    int8_t track_error; // 弧线循迹偏差，左负右正
    uint8_t track_valid; // 当前是否有可用循迹偏差
    float yaw_deg; // MPU6050 积分得到的当前相对航向角
    float target_yaw_deg; // 当前直线路段需要保持的航向角
    float distance_cm; // 当前路段已经行驶的距离
} H2024Telemetry;

void h2024_task_loop(void); // 按键选择并执行 2024 H 题路线，作为 FreeRTOS 任务主体
void h2024_get_telemetry(H2024Telemetry *data); // 获取串口打印使用的任务状态快照

#endif
