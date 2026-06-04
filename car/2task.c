#include "2task.h"
#include "button.h"
#include "oled.h"
#include "pid.h"
#include "projdefs.h"
#include "track.h"
#include <stdio.h>

SemaphoreHandle_t            button_sem;
volatile uint8_t             button_task;
extern PID                   pid_left;
extern PID                   pid_right;

static volatile TrackInfo line_follow_track_info; // 循迹任务保存给遥测显示任务使用

void OLED(void *pvParameters)
{
    char text[22]; // 128 像素宽度下 6x8 字体最多显示 21 个 ASCII 字符
    ControlTelemetry data; // 保存控制模块提供的状态快照
    TickType_t last_wake_time = xTaskGetTickCount(); // 记录固定周期起点
    (void) pvParameters; // 当前任务不需要入口参数

    OLED_Clear();

    while (1) {
        control_get_telemetry(&data); // 获取最新左右轮和循迹数据
        taskENTER_CRITICAL();
        data.track_raw = line_follow_track_info.raw;
        data.track_error = line_follow_track_info.error;
        data.track_valid = line_follow_track_info.valid;
        taskEXIT_CRITICAL();

        snprintf(text, sizeof(text), "RPM L%4ld R%4ld    ",
            (long) data.left_rpm,
            (long) data.right_rpm);
        OLED_ShowString(0, 0, (uint8_t *) text, 8);

        snprintf(text, sizeof(text), "PWM L%4ld R%4ld    ",
            (long) data.left_pwm,
            (long) data.right_pwm);
        OLED_ShowString(0, 1, (uint8_t *) text, 8);

        snprintf(text, sizeof(text), "CNT L%4ld R%4ld    ",
            (long) data.left_count,
            (long) data.right_count);
        OLED_ShowString(0, 2, (uint8_t *) text, 8);

        snprintf(text, sizeof(text), "RAW 0x%02X          ",
            (unsigned int) data.track_raw);
        OLED_ShowString(0, 3, (uint8_t *) text, 8);

        snprintf(text, sizeof(text), "TRK %c%c%c%c%c%c%c%c       ",
            ((data.track_raw & (1U << 0)) != 0U) ? '1' : '0',
            ((data.track_raw & (1U << 1)) != 0U) ? '1' : '0',
            ((data.track_raw & (1U << 2)) != 0U) ? '1' : '0',
            ((data.track_raw & (1U << 3)) != 0U) ? '1' : '0',
            ((data.track_raw & (1U << 4)) != 0U) ? '1' : '0',
            ((data.track_raw & (1U << 5)) != 0U) ? '1' : '0',
            ((data.track_raw & (1U << 6)) != 0U) ? '1' : '0',
            ((data.track_raw & (1U << 7)) != 0U) ? '1' : '0');
        OLED_ShowString(0, 4, (uint8_t *) text, 8);

        snprintf(text, sizeof(text), "ERR %4d V%u        ",
            (int) data.track_error,
            (unsigned int) data.track_valid);
        OLED_ShowString(0, 5, (uint8_t *) text, 8);

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(TELEMETRY_PERIOD_MS)); // 每 200ms 刷新一次
    }
}

void Line_Follow(void *pvParameters)
{
    uint32_t track_age_ms = TIMEOUT_MS;                                  // 上电还没收到循迹帧，先按超时处理
    TickType_t last_wake_time = xTaskGetTickCount();                     // 记录循迹任务的固定周期起点
    int8_t last_error = 0;                                                // 上一次循迹偏差，给 D 项使用
    float last_turn = 0.0f;                                               // 上一次循迹修正量，用来做简单平滑

    (void) pvParameters;

    while (1) {
        if (Track_receive() != 0U) {
            track_age_ms = 0U;                                           // 收到新帧就清零超时计数
        } else if (track_age_ms < TIMEOUT_MS) {
            track_age_ms += LOOP_MS;                                     // 没收到新帧就累计等待时间
        }

        TrackInfo info = Track_get_info();                               // 获取最新 raw 和循迹偏差
        taskENTER_CRITICAL();
        line_follow_track_info.raw = info.raw;
        line_follow_track_info.error = info.error;
        line_follow_track_info.valid = info.valid;
        taskEXIT_CRITICAL();

        if ((info.valid == 0U) || (track_age_ms >= TIMEOUT_MS)) {
            taskENTER_CRITICAL();
            pid_set_target(&pid_left, 0.0f);                              // 没有可靠循迹数据时左右轮停车
            pid_set_target(&pid_right, 0.0f);
            taskEXIT_CRITICAL();
            last_error = 0;                                               // 清掉循迹 D 项历史偏差，恢复后重新计算
            last_turn = 0.0f;                                             // 清掉转向平滑量，恢复后避免突然打一把
        } else {
            float p = (float) info.error * LINE_FOLLOW_KP;                 // P 项：当前位置偏差带来的修正
            float d = (float) (info.error - last_error) *
                LINE_FOLLOW_KD;                                          // D 项：偏差变化带来的修正
            float turn = p + d;                                          // 计算转向差速

            if (turn > TURN_RPM) {
                turn = TURN_RPM;
            } else if (turn < -TURN_RPM) {
                turn = -TURN_RPM;
            }

            turn = 0.75f * turn + 0.25f * last_turn;                     // 对转向输出做一点平滑
            last_error = info.error;
            last_turn = turn;

            taskENTER_CRITICAL();
            pid_set_target(&pid_left, BASE_RPM + turn);                  // 更新左右轮 RPM
            pid_set_target(&pid_right, BASE_RPM - turn);
            taskEXIT_CRITICAL();
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(LOOP_MS));         // 每 5ms 唤醒一次，不阻塞其他 FreeRTOS 任务
    }

    vTaskDelete(NULL); // 理论上不会执行到这里
}

void KEY(void *pvParameters)
{
    (void) pvParameters;

    while (1) {
        if (xSemaphoreTake(button_sem, portMAX_DELAY) == pdTRUE) {   //等信号量
            vTaskDelay(pdMS_TO_TICKS(20));                          //消抖

            if (Button_is_pressed()) {
                Button_switch();                                    //切换下一个任务模式

                while (Button_is_pressed()) {
                    vTaskDelay(pdMS_TO_TICKS(10));                  //等松手
                }
                vTaskDelay(pdMS_TO_TICKS(10));                      //松手消抖
            }
        }
    }
}
