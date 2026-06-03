#include "app_main.h"

#include "control.h"
#include "uart.h"

#include "FreeRTOS.h"
#include "task.h"

#include <stdint.h>
#include <stdio.h>

void app_main(void)
{
    BaseType_t result; // 保存 FreeRTOS 任务创建结果

    result = xTaskCreate(
        Line_Follow, 
        "follow",
        configMINIMAL_STACK_SIZE * 2U, NULL,
        tskIDLE_PRIORITY + 2U, NULL); // 循迹任务优先级较高，每 5ms 更新左右轮目标速度
    configASSERT(result == pdPASS); // 创建失败时停在断言处

    result = xTaskCreate(
        UART, 
        "uart",
        configMINIMAL_STACK_SIZE * 3U, NULL,
        tskIDLE_PRIORITY + 1U, NULL); // 串口监视任务优先级较低，避免影响循迹
    configASSERT(result == pdPASS); // 创建失败时停在断言处

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
