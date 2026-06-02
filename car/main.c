/* TI includes */
#include "ti_msp_dl_config.h"
#include "ti/driverlib/dl_timer.h"

#include "app_main.h"
#include "control.h"
#include "encode.h"
#include "motor.h"
#include "uart.h"

int main(void)
{
    SYSCFG_DL_init();                               // 初始化 syscfg 里配置好的 GPIO、PWM、Timer、UART 等外设

    motor_init();                                   // 初始化 TB6612 方向脚、STBY 使能脚和 PWM 输出值
    encoder_init();                                 // 初始化编码器 AB 相初始状态，并打开 GPIO 中断
    UART_init();                                    // 打开 UART 接收中断，用于接收副板发送的八路灰度 raw

    control_init();                                 // 初始化左右轮速度环，默认目标速度为 0

    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);    // 清掉可能残留的 PID 定时器中断标志
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);          // 允许 TIMER_0 进入中断，PID 才会周期执行,周期10ms

    DL_Timer_startCounter(PWM_0_INST);              // 启动 PWM_0，PWMA/PWMB 才会输出占空比
    DL_Timer_startCounter(TIMER_0_INST);            // 启动 TIMER_0，固定周期读取编码器并算 PID

    app_main();                                     // 创建循迹和串口监视任务，并启动 FreeRTOS 调度器

    return 0;
}
