#include "uart.h"

volatile uint8_t uart_data[UART_RX_BUFFER_SIZE];
volatile uint16_t uart_rx_index = 0;
SemaphoreHandle_t UART_xSemaphore;
void uart_init(void)
{
  UART_xSemaphore = xSemaphoreCreateBinary();
  NVIC_ClearPendingIRQ(UART_0_INST_INT_IRQN);//清空串口标志位
  NVIC_EnableIRQ(UART_0_INST_INT_IRQN);//启动串口中断
}


//串口发送单个字符,阻塞发送
void uart0_send_char(char ch)
{
    //当串口0忙的时候等待，不忙的时候再发送传进来的字符
    while( DL_UART_isBusy(UART_0_INST) == true );
    //发送单个字符
    DL_UART_Main_transmitData(UART_0_INST, ch);
}
//串口发送字符串
void uart0_send_string(char* str)
{
    //当前字符串地址不在结尾 并且 字符串首地址不为空
    while(*str!=0&&str!=0)
    {
        //发送字符串首地址中的字符，并且在发送完成之后首地址自增
        uart0_send_char(*str++);
    }
}

//串口的中断服务函数
void UART_0_INST_IRQHandler(void)
{
    switch( DL_UART_getPendingInterrupt(UART_0_INST) )
    {
        case DL_UART_IIDX_RX://如果是接收中断
            // 接发送过来的数据保存在变量中
            DL_UART_receiveDataCheck(UART_0_INST, (uint8_t *)(uart_data + uart_rx_index));
            uart_rx_index++;
            break;
        case DL_UART_IIDX_RX_TIMEOUT_ERROR:
            while(DL_UART_receiveDataCheck(UART_0_INST, (uint8_t *)(uart_data + uart_rx_index)))
            {
                uart_rx_index++;
            }
            uart0_send_string((char *)uart_data);
            uart_rx_index = 0;
            memset((void *)uart_data, 0, sizeof(uart_data));
            // xSemaphoreGive(UART_xSemaphore);
            break;
        default://其他的串口中断
            break;
    }
    return;
}
