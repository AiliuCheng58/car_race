#include "app_main.h"

#include "control.h"
#include "uart.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>
#include <stdio.h>

#define TELEMETRY_PERIOD_MS (200U) // 串口监视输出周期，过快会占用较多串口带宽

static void line_follow_task(void *argument)
{
    (void) argument; // 当前任务不需要入口参数
    control_line_follow_loop(); // 普通循迹循环会一直运行
    vTaskDelete(NULL); // 理论上不会执行到这里
}

static void telemetry_task(void *argument)
{
    char text[180]; // 保存一行串口监视文本
    ControlTelemetry data; // 保存控制模块提供的状态快照
    TickType_t last_wake_time = xTaskGetTickCount(); // 记录固定周期起点

    (void) argument; // 当前任务不需要入口参数
    UART_SendString("lcnt,rcnt,lrpm,rrpm,lpwm,rpwm,raw,track,error,valid\r\n"); // 打印列名

    while (1) {
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

        UART_SendString(text); // 把本轮状态发到串口助手
        vTaskDelayUntil(&last_wake_time,
            pdMS_TO_TICKS(TELEMETRY_PERIOD_MS)); // 每 200ms 打印一次
    }
}

void app_main(void)
{
    BaseType_t result; // 保存 FreeRTOS 任务创建结果

    result = xTaskCreate(line_follow_task, "track",
        configMINIMAL_STACK_SIZE * 2U, NULL,
        tskIDLE_PRIORITY + 2U, NULL); // 循迹任务优先级较高，每 5ms 更新左右轮目标速度
    configASSERT(result == pdPASS); // 创建失败时停在断言处，便于调试

    result = xTaskCreate(telemetry_task, "telemetry",
        configMINIMAL_STACK_SIZE * 3U, NULL,
        tskIDLE_PRIORITY + 1U, NULL); // 串口监视任务优先级较低，避免影响循迹
    configASSERT(result == pdPASS); // 创建失败时停在断言处，便于调试

    vTaskStartScheduler(); // 启动 FreeRTOS 调度器

    while (1) { // 正常情况下调度器启动后不会返回
    }
}

#if (configSUPPORT_STATIC_ALLOCATION == 1)
void vApplicationGetIdleTaskMemory(StaticTask_t **task_buffer,
    StackType_t **stack_buffer, configSTACK_DEPTH_TYPE *stack_size)
{
    static StaticTask_t idle_task_buffer; // Idle 任务控制块
    static StackType_t idle_stack[configMINIMAL_STACK_SIZE]; // Idle 任务栈

    *task_buffer = &idle_task_buffer; // 返回 Idle 任务控制块地址
    *stack_buffer = idle_stack; // 返回 Idle 任务栈地址
    *stack_size = configMINIMAL_STACK_SIZE; // 返回 Idle 任务栈大小
}

#if (configUSE_TIMERS == 1)
void vApplicationGetTimerTaskMemory(StaticTask_t **task_buffer,
    StackType_t **stack_buffer, configSTACK_DEPTH_TYPE *stack_size)
{
    static StaticTask_t timer_task_buffer; // 软件定时器任务控制块
    static StackType_t timer_stack[configTIMER_TASK_STACK_DEPTH]; // 软件定时器任务栈

    *task_buffer = &timer_task_buffer; // 返回 Timer 任务控制块地址
    *stack_buffer = timer_stack; // 返回 Timer 任务栈地址
    *stack_size = configTIMER_TASK_STACK_DEPTH; // 返回 Timer 任务栈大小
}
#endif
#endif

void vApplicationStackOverflowHook(TaskHandle_t task, char *task_name)
{
    (void) task; // 调试时可在这里查看溢出的任务句柄
    (void) task_name; // 调试时可在这里查看溢出的任务名

    taskDISABLE_INTERRUPTS(); // 栈溢出后停止继续运行，避免产生不可预测行为
    while (1) {
    }
}
