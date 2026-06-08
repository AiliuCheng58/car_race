#include "2task.h"
#include "button.h"
#include "control.h"
#include "oled.h"
#include "track.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>

#define LINE_FOLLOW_BASE_RPM            (100.0f) // 基础前进速度，偏差为 0 时左右轮都跑这个 RPM
#define LINE_FOLLOW_KP                  (10.0f)  // 循迹 P 系数，偏差越大，左右轮速度差越大
#define LINE_FOLLOW_KD                  (2.0f)   // 循迹 D 系数，偏差变化越快，提前加一点修正
#define LINE_FOLLOW_TURN_RPM            (100.0f) // 循迹修正量限幅，避免一偏就原地打转
#define LINE_FOLLOW_PERIOD_MS           (5U)     // 循迹任务刷新周期，单位 ms
#define LINE_FOLLOW_TIMEOUT_MS          (500U)   // 超过这个时间没有新循迹帧就停车
#define OLED_TELEMETRY_PERIOD_MS        (200U)   // 遥测显示刷新周期，过快会占用较多任务时间

SemaphoreHandle_t            button_sem;
volatile uint8_t             button_task;

static volatile TrackInfo line_follow_track_info; // 循迹任务保存给遥测显示任务使用

void Line_Follow(void *pvParameters)
{
    
    uint32_t track_age_ms = LINE_FOLLOW_TIMEOUT_MS;                      // 上电还没收到循迹帧，先按超时处理
    TickType_t last_wake_time = xTaskGetTickCount();                     // 记录循迹任务的固定周期起点
    int8_t last_error = 0;                                                // 上一次循迹偏差，给 D 项使用
    float last_turn = 0.0f;                                               // 上一次循迹修正量，用来做简单平滑

    (void) pvParameters;

    while (1) {
        if (button_task != BUTTON_TASK_LINE_FOLLOW) {
            control_stop();                                               // 未按键进入循迹模式前保持停车
            track_age_ms = LINE_FOLLOW_TIMEOUT_MS;                        // 进入循迹后先等待新帧
            last_error = 0;                                               // 清掉循迹 D 项历史偏差
            last_turn = 0.0f;                                             // 清掉转向平滑量

            taskENTER_CRITICAL();
            line_follow_track_info.raw = 0U;
            line_follow_track_info.error = 0;
            line_follow_track_info.valid = 0U;
            taskEXIT_CRITICAL();

            vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(LINE_FOLLOW_PERIOD_MS));
            continue;
        }

        if (Track_receive() != 0U) {
            track_age_ms = 0U;                                           // 收到新帧就清零超时计数
        } else if (track_age_ms < LINE_FOLLOW_TIMEOUT_MS) {
            track_age_ms += LINE_FOLLOW_PERIOD_MS;                       // 没收到新帧就累计等待时间
        }

        TrackInfo info = Track_get_info();                               // 获取最新 raw 和循迹偏差
        taskENTER_CRITICAL();
        line_follow_track_info.raw = info.raw;
        line_follow_track_info.error = info.error;
        line_follow_track_info.valid = info.valid;
        taskEXIT_CRITICAL();

        if ((info.valid == 0U) || (track_age_ms >= LINE_FOLLOW_TIMEOUT_MS)) {
            control_stop();                                               // 没有可靠循迹数据时左右轮停车
            last_error = 0;                                               // 清掉循迹 D 项历史偏差，恢复后重新计算
            last_turn = 0.0f;                                             // 清掉转向平滑量，恢复后避免突然打一把
        } else {
            float p = (float) info.error * LINE_FOLLOW_KP;                 // P 项：当前位置偏差带来的修正
            float d = (float) (info.error - last_error) * LINE_FOLLOW_KD;  // D 项：偏差变化带来的修正
            float turn = p + d;                                          // 计算转向差速

            if (turn > LINE_FOLLOW_TURN_RPM) {
                turn = LINE_FOLLOW_TURN_RPM;
            } else if (turn < -LINE_FOLLOW_TURN_RPM) {
                turn = -LINE_FOLLOW_TURN_RPM;
            }

            turn = 0.75f * turn + 0.25f * last_turn;                     // 对转向输出做一点平滑
            last_error = info.error;
            last_turn = turn;

            control_set_target_rpm(
                LINE_FOLLOW_BASE_RPM - turn,
                LINE_FOLLOW_BASE_RPM + turn);                            // 更新左右轮目标 RPM
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(LINE_FOLLOW_PERIOD_MS)); // 每 5ms 唤醒一次，不阻塞其他 FreeRTOS 任务
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

void OLED(void *pvParameters)
{
    char text[22]; // 128 像素宽度下 6x8 字体最多显示 21 个 ASCII 字符
    ControlTelemetry data; // 保存控制模块提供的状态快照
    TrackInfo track; // 保存循迹任务提供的状态快照
    TickType_t last_wake_time = xTaskGetTickCount(); // 记录固定周期起点
    (void) pvParameters; // 当前任务不需要入口参数

    OLED_Clear();

    while (1) {
        control_get_telemetry(&data); // 获取最新左右轮闭环数据
        taskENTER_CRITICAL();
        track.raw = line_follow_track_info.raw;
        track.error = line_follow_track_info.error;
        track.valid = line_follow_track_info.valid;
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
            (unsigned int) track.raw);
        OLED_ShowString(0, 3, (uint8_t *) text, 8);

        snprintf(text, sizeof(text), "TRK %c%c%c%c%c%c%c%c       ",
            ((track.raw & (1U << 0)) != 0U) ? '1' : '0',
            ((track.raw & (1U << 1)) != 0U) ? '1' : '0',
            ((track.raw & (1U << 2)) != 0U) ? '1' : '0',
            ((track.raw & (1U << 3)) != 0U) ? '1' : '0',
            ((track.raw & (1U << 4)) != 0U) ? '1' : '0',
            ((track.raw & (1U << 5)) != 0U) ? '1' : '0',
            ((track.raw & (1U << 6)) != 0U) ? '1' : '0',
            ((track.raw & (1U << 7)) != 0U) ? '1' : '0');
        OLED_ShowString(0, 4, (uint8_t *) text, 8);

        snprintf(text, sizeof(text), "ERR %4d V%u        ",
            (int) track.error,
            (unsigned int) track.valid);
        OLED_ShowString(0, 5, (uint8_t *) text, 8);

        snprintf(text, sizeof(text), "OLED RUN %lu       ",
            (unsigned long) (xTaskGetTickCount() / pdMS_TO_TICKS(1000U)));
        OLED_ShowString(0, 6, (uint8_t *) text, 8);

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(OLED_TELEMETRY_PERIOD_MS)); // 每 200ms 刷新一次
    }
}
