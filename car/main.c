/* TI includes */
#include "button.h"
#include "ti_msp_dl_config.h"
#include "ti/driverlib/dl_timer.h"

#include "app_main.h"
#include "control.h"
#include "delay.h"
#include "encode.h"
#include "motor.h"
#include "oled.h"
#include "uart.h"
#include "odom/odom.h"

int main(void)
{
    SYSCFG_DL_init();                               // 初始化 syscfg 里配置好的 GPIO、PWM、Timer、UART 等外设

    motor_init();                                   // 初始化 TB6612 方向脚、STBY 使能脚和 PWM 输出值
    Button_init();                                  // 按键初始化，先创建信号量再打开 GPIOB 中断
    encoder_init();                                 // 初始化编码器 AB 相初始状态，并打开 GPIO 中断
    UART_init();                                    // 打开 UART 接收中断，后面用于循迹数据和 PID 串口调参
    /*OLED_Init();                                    // 初始化板载显示接口上的 4 针 I2C OLED
    OLED_WR_Byte(0xA5, OLED_CMD);                   // 全屏点亮 1 秒，用来确认 OLED 通信和供电是否正常
    delay_ms(1000U);
    OLED_WR_Byte(0xA4, OLED_CMD);
    OLED_Clear();
    OLED_ShowString(0, 0, (uint8_t *) "OLED BOOT", 8);
    OLED_ShowString(0, 1, (uint8_t *) "SCL PB9 SDA PB8", 8);
*/
    odom_init();                                    // 初始化里程计
    control_init();                                 // 初始化左右轮速度环，默认目标速度为 0

    NVIC_ClearPendingIRQ(TIMER_0_INST_INT_IRQN);    // 清掉可能残留的 PID 定时器中断标志
    NVIC_EnableIRQ(TIMER_0_INST_INT_IRQN);          // 允许 TIMER_0 进入中断，PID 才会周期执行,周期10ms

    DL_Timer_startCounter(PWM_0_INST);              // 启动 PWM_0，PWMA/PWMB 才会输出占空比
    DL_Timer_startCounter(TIMER_0_INST);            // 启动 TIMER_0，固定周期读取编码器并算 PID

    app_main();                                     // 创建循迹、OLED 显示和按键任务，并启动 FreeRTOS 调度器

    return 0;
}
