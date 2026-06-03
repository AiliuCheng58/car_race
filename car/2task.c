#include "2task.h"
#define TELEMETRY_PERIOD_MS (200U) // 串口监视输出周期，过快会占用较多串口带宽

void UART(void *pvParameters)
{
    char text[180]; // 保存一行串口监视文本
    ControlTelemetry data; // 保存控制模块提供的状态快照
    TickType_t last_wake_time = xTaskGetTickCount(); // 记录固定周期起点
    (void) pvParameters; // 当前任务不需要入口参数
    while (1) {
        UART_SendString("lcnt,rcnt,lrpm,rrpm,lpwm,rpwm,raw,track,error,valid\r\n"); // 打印列名
        control_get_telemetry(&data); // 获取最新左右轮和循迹数据

        snprintf(text, sizeof(text),
            "%ld,%ld,%ld,%ld,%ld,%ld,0x%02X,%c%c%c%c%c%c%c%c,%d,%u\r\n",
            (long) data.left_count, // 左轮最近 10ms 编码器计数
            (long) data.right_count, // 右轮最近 10ms 编码器计数
            (long) data.left_rpm, // 左轮当前 RPM，串口中取整数便于观察
            (long) data.right_rpm, // 右轮当前 RPM
            (long) data.left_pwm, // 左轮当前带符号 PWM
            (long) data.right_pwm, // 右轮当前带符号 PWM
            (unsigned int) data.track_raw, // 八路循迹原始十六进制值
            ((data.track_raw & (1U << 0)) != 0U) ? '1' : '0', // bit0：最左侧探头
            ((data.track_raw & (1U << 1)) != 0U) ? '1' : '0',
            ((data.track_raw & (1U << 2)) != 0U) ? '1' : '0',
            ((data.track_raw & (1U << 3)) != 0U) ? '1' : '0',
            ((data.track_raw & (1U << 4)) != 0U) ? '1' : '0',
            ((data.track_raw & (1U << 5)) != 0U) ? '1' : '0',
            ((data.track_raw & (1U << 6)) != 0U) ? '1' : '0',
            ((data.track_raw & (1U << 7)) != 0U) ? '1' : '0', // bit7：最右侧探头
            (int) data.track_error, // 循迹偏差，左负右正
            (unsigned int) data.track_valid); // 当前是否有可用循迹偏差

        UART_SendString(text);
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(TELEMETRY_PERIOD_MS)); // 每 200ms 打印一次
    }
}

void Line_Follow(void *pvParameters)
{
    (void) pvParameters;
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

    vTaskDelete(NULL); // 理论上不会执行到这里
}

