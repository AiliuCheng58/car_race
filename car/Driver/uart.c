#include "uart.h"

#define UART_RX_ERROR_MASK (DL_UART_INTERRUPT_OVERRUN_ERROR | \
                            DL_UART_INTERRUPT_PARITY_ERROR | \
                            DL_UART_INTERRUPT_FRAMING_ERROR | \
                            DL_UART_INTERRUPT_NOISE_ERROR) // 接收异常统一处理，避免 UART 卡在错误状态

volatile uint8_t UART_Data[UART_RX_BUFFER_SIZE]; // 串口接收缓存，当前只保存循迹模块数据
volatile uint16_t RX_index = 0; // 当前缓存里已经收到的字节数

static void UART_SaveByte(uint8_t data)
{
    if (RX_index >= UART_RX_BUFFER_SIZE) { // 缓存已满，说明此前一直没有解析到合法帧
        RX_index = 0U; // 从头重新收，后续 Track_receive() 会重新寻找帧头
    }

    UART_Data[RX_index++] = data; // 把新字节追加到缓存尾部
}

void UART_init(void)
{
  DL_UART_clearInterruptStatus(UART_0_INST,
      DL_UART_INTERRUPT_RX | UART_RX_ERROR_MASK); // 清掉上电时可能残留的接收和错误标志
  DL_UART_enableInterrupt(UART_0_INST, UART_RX_ERROR_MASK); // 除 RX 外，再打开错误中断用于自动恢复
  NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN); // 清空 UART0 在 NVIC 里的挂起中断
  NVIC_EnableIRQ(UART_0_INST_INT_IRQN); // 允许 UART0 接收中断进入 ISR
}

void UART_MoveBytes(uint16_t count)
{
    uint32_t primask = __get_PRIMASK(); // 保存进入函数前的中断状态

    __disable_irq(); // 移动缓存时避免 UART 中断同时写入

    if (count >= RX_index) {
        RX_index = 0U; // 要消费的字节数超过缓存长度时直接清空
        memset((void *) UART_Data, 0, UART_RX_BUFFER_SIZE); // 清掉旧数据，避免下次误解析
    } else {
        RX_index -= count;
        memmove((void *) UART_Data, (const void *) &UART_Data[count],
            RX_index); // 把还没处理的数据前移到缓存头
        memset((void *) &UART_Data[RX_index], 0,
            UART_RX_BUFFER_SIZE - RX_index); // 清掉尾部残留数据
    }

    __set_PRIMASK(primask); // 恢复原来的中断状态
}

//串口发送字符串
void UART_SendString(char *str)
{
    //当前字符串地址不在结尾 并且 字符串首地址不为空
    while(str != NULL && *str != '\0') // 字符串指针有效且还没到 '\0'
    {
        //发送字符串首地址中的字符，并且在发送完成之后首地址自增
        DL_UART_transmitDataBlocking(UART_0_INST, *str);
        str++; // 移动到下一个字符，否则会一直重复发送第一个字符
    }
}

//串口的中断服务函数
void UART_0_INST_IRQHandler(void)
{
    uint32_t error_status = DL_UART_getEnabledInterruptStatus(
        UART_0_INST, UART_RX_ERROR_MASK); // 先检查是否发生溢出、校验、帧或噪声错误
    uint8_t data; // 暂存从硬件 FIFO 读出的一个字节

    if (error_status != 0U) { // 接收过程中发生异常
        DL_UART_clearInterruptStatus(UART_0_INST, error_status); // 清错误标志，让后续字节可以继续触发中断
        RX_index = 0U; // 当前帧可能已经残缺，直接丢弃并等待下一个 0xFE
    }

    while (DL_UART_receiveDataCheck(UART_0_INST, &data) == true) { // 一次中断把 FIFO 里已有字节全部取出
        UART_SaveByte(data); // 追加到软件缓存，后面由 Track_receive() 解析完整帧
    }

    DL_UART_clearInterruptStatus(UART_0_INST, DL_UART_INTERRUPT_RX); // 清接收中断标志
}
