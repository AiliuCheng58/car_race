#include "2task.h"
#include "button/button.h"
#include "control/control.h"
#include "line.h"
#include "odom/odom.h"
#include "oled.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdio.h>

#define LINE_FOLLOW_BASE_RPM     (100.0f) // 基础前进速度
#define LINE_FOLLOW_KP           (10.0f)  // 循迹 P 系数
#define LINE_FOLLOW_KI           (0.0f)   // 循迹 I 系数，先置零方便调参
#define LINE_FOLLOW_KD           (2.0f)   // 循迹 D 系数
#define LINE_FOLLOW_TURN_RPM     (100.0f) // 转向差速限幅
#define LINE_FOLLOW_I_MAX        (50.0f)  // 循迹积分累计限幅
#define OLED_TELEMETRY_PERIOD_MS (200U)   // OLED 刷新周期

#define ROAD_WHITE               (1.281f) // 白区斜线长度
#define LINE_FOLLOW_MIN_DIST     (0.3f)   // FOLLOW 最小有效距离
#define ANGLE_KP                 (5.0f)   // 角度 P 系数
#define ANGLE_KI                 (0.0f)   // 角度 I 系数，先保留结构
#define ANGLE_KD                 (1.0f)   // 角度 D 系数
#define ANGLE_I_MAX              (50.0f)  // 角度积分累计限幅

SemaphoreHandle_t button_sem;
volatile uint8_t button_task;

static volatile RaceSegment SEG = SEG_A2C; // 状态机初始状态

void Line_Follow(void *pvParameters)
{
    LineTrack track;                              // 三个状态共用的循迹数据快照
    TickType_t last_wake_time = xTaskGetTickCount();
    int8_t last_error = 0;                        // 循迹 D 项历史误差
    uint8_t edge_armed = 0U;                      // 进入白区后才允许锁存上升沿
    uint8_t edge_latched = 0U;                    // 白区后的上升沿锁存
    uint8_t angle_ready = 0U;                     // 角度 D 项是否已初始化
    uint8_t follow_exit_count = 0U;               // FOLLOW 出口次数
    float last_turn = 0.0f;                       // 上一次转向输出
    float p, i, d, turn, target_angle;
    float line_i = 0.0f;                          // 循迹环积分累计
    float angle_i = 0.0f;                         // 角度环积分累计
    float last_angle_err = 0.0f;
    float angle_err = 0.0f;

    (void) pvParameters;
    Line_track_reset(&track);

    while (1) {
        if (button_task != BUTTON_TASK_LINE_FOLLOW) {
            control_stop();                       // 未进入循迹模式前保持停车
            SEG = SEG_A2C;                        // 下次从 A2C 重新开始
            edge_armed = 0U;
            edge_latched = 0U;
            angle_ready = 0U;
            follow_exit_count = 0U;
            last_error = 0;
            line_i = 0.0f;
            angle_i = 0.0f;
            last_angle_err = 0.0f;
            last_turn = 0.0f;
            Line_track_reset(&track);
            vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(LINE_FOLLOW_PERIOD_MS));
            continue;
        }

        Line_track_read(&track);                  // 三个状态共用同一份循迹数据

        switch (SEG) {
            case SEG_A2C:
            case SEG_B2D:
                odom_update();
                target_angle = (SEG == SEG_A2C) ? odom.target_A2C : odom.target_B2D;
                angle_err = odom_wrap180(target_angle - odom.theta);
                if (angle_ready == 0U) {          // 进入斜线段第一帧不产生 D 尖峰
                    last_angle_err = angle_err;
                    angle_i = 0.0f;
                    angle_ready = 1U;
                }

                angle_i += angle_err * ((float) LINE_FOLLOW_PERIOD_MS / 1000.0f);
                if (angle_i > ANGLE_I_MAX)
                    angle_i = ANGLE_I_MAX;
                else if (angle_i < -ANGLE_I_MAX)
                    angle_i = -ANGLE_I_MAX;
                p = angle_err * ANGLE_KP;
                i = angle_i * ANGLE_KI;
                d = (angle_err - last_angle_err) * ANGLE_KD;
                turn = p + i + d;
                if (turn > LINE_FOLLOW_TURN_RPM)
                    turn = LINE_FOLLOW_TURN_RPM;
                else if (turn < -LINE_FOLLOW_TURN_RPM)
                    turn = -LINE_FOLLOW_TURN_RPM;

                turn = 0.75f * turn + 0.25f * last_turn;
                last_angle_err = angle_err;
                last_turn = turn;
                control_set_target_rpm(LINE_FOLLOW_BASE_RPM - turn, LINE_FOLLOW_BASE_RPM + turn);

                if (track.on_line == 0U)
                    edge_armed = 1U;              // 确认已经离开起点黑线
                if ((edge_armed != 0U) && Line_track_rise(&track))
                    edge_latched = 1U;            // 上升沿只要出现过就锁存
                if ((odom.distance >= ROAD_WHITE) && (edge_latched != 0U)) {
                    SEG = SEG_FOLLOW;
                    odom_clear();
                    edge_armed = 0U;
                    edge_latched = 0U;
                    angle_ready = 0U;
                    last_error = 0;
                    line_i = 0.0f;
                    angle_i = 0.0f;
                    last_angle_err = 0.0f;
                    last_turn = 0.0f;
                }
                break;

            case SEG_FOLLOW:
                odom_update();
                if (Line_track_lost(&track)) {
                    control_stop();               // 循迹串口超时才真正停车
                    last_error = 0;
                    line_i = 0.0f;
                    last_turn = 0.0f;
                } else {
                    line_i += (float) track.info.error * ((float) LINE_FOLLOW_PERIOD_MS / 1000.0f);
                    if (line_i > LINE_FOLLOW_I_MAX)
                        line_i = LINE_FOLLOW_I_MAX;
                    else if (line_i < -LINE_FOLLOW_I_MAX)
                        line_i = -LINE_FOLLOW_I_MAX;

                    p = (float) track.info.error * LINE_FOLLOW_KP;
                    i = line_i * LINE_FOLLOW_KI;
                    d = (float) (track.info.error - last_error) * LINE_FOLLOW_KD;
                    turn = p + i + d;
                    if (turn > LINE_FOLLOW_TURN_RPM)
                        turn = LINE_FOLLOW_TURN_RPM;
                    else if (turn < -LINE_FOLLOW_TURN_RPM)
                        turn = -LINE_FOLLOW_TURN_RPM;
                    turn = 0.75f * turn + 0.25f * last_turn;
                    last_error = track.info.error;
                    last_turn = turn;
                    control_set_target_rpm(LINE_FOLLOW_BASE_RPM - turn, LINE_FOLLOW_BASE_RPM + turn);
                }

                if (Line_track_fall(&track) && (odom.distance >= LINE_FOLLOW_MIN_DIST)) {
                    odom_recalibrate_targets();   // 在弧线段入口用当前 yaw 更新目标角
                    SEG = ((follow_exit_count & 1U) == 0U) ? SEG_B2D : SEG_A2C;
                    follow_exit_count++;
                    odom_clear();
                    edge_armed = 0U;
                    edge_latched = 0U;
                    angle_ready = 0U;
                    last_error = 0;
                    line_i = 0.0f;
                    angle_i = 0.0f;
                    last_angle_err = 0.0f;
                    last_turn = 0.0f;
                }
                break;

            default:
                break;
        }

        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(LINE_FOLLOW_PERIOD_MS));
    }
}

void KEY(void *pvParameters)
{
    (void) pvParameters;

    while (1) {
        if (xSemaphoreTake(button_sem, portMAX_DELAY) == pdTRUE) { // 等信号量
            vTaskDelay(pdMS_TO_TICKS(20));                         // 消抖
            if (Button_is_pressed()) {
                Button_switch();                                   // 切换下一个任务模式
                while (Button_is_pressed())
                    vTaskDelay(pdMS_TO_TICKS(10));                 // 等松手
                vTaskDelay(pdMS_TO_TICKS(10));                     // 松手消抖
            }
        }
    }
}

void OLED(void *pvParameters)
{
    char text[22];                          // 6x8 字体最多显示 21 个 ASCII 字符
    ControlTelemetry data;                  // 控制模块状态快照
    TrackInfo track;                        // 循迹状态快照
    TickType_t last_wake_time = xTaskGetTickCount();
    (void) pvParameters;

    OLED_Clear();

    while (1) {
        control_get_telemetry(&data);
        track = Line_track_info();

        snprintf(text, sizeof(text), "RPM L%4ld R%4ld    ", (long) data.left_rpm, (long) data.right_rpm);
        OLED_ShowString(0, 0, (uint8_t *) text, 8);
        snprintf(text, sizeof(text), "PWM L%4ld R%4ld    ", (long) data.left_pwm, (long) data.right_pwm);
        OLED_ShowString(0, 1, (uint8_t *) text, 8);
        snprintf(text, sizeof(text), "CNT L%4ld R%4ld    ", (long) data.left_count, (long) data.right_count);
        OLED_ShowString(0, 2, (uint8_t *) text, 8);
        snprintf(text, sizeof(text), "RAW 0x%02X          ", (unsigned int) track.raw);
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
        snprintf(text, sizeof(text), "ERR %4d V%u        ", (int) track.error, (unsigned int) track.valid);
        OLED_ShowString(0, 5, (uint8_t *) text, 8);
        snprintf(text, sizeof(text), "OLED RUN %lu       ", (unsigned long) (xTaskGetTickCount() / pdMS_TO_TICKS(1000U)));
        OLED_ShowString(0, 6, (uint8_t *) text, 8);
        vTaskDelayUntil(&last_wake_time, pdMS_TO_TICKS(OLED_TELEMETRY_PERIOD_MS));
    }
}
