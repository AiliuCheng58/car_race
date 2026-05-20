#include "uart.h"

#include "ti/driverlib/dl_uart_main.h"
#include "ti_msp_dl_config.h"

uint8_t uart_SendByte(uint8_t data){
    uint32_t timeout;

    // 等待 TX FIFO 有空闲空间
    timeout = UART_TIMEOUT;
    while (DL_UART_isTXFIFOFull(UART_0_INST)){
        if (--timeout == 0)
            return 0;
    }

    // 写入 TX FIFO
    DL_UART_transmitData(UART_0_INST, data);

    // 等待 UART 发送完成 (总线空闲)
    timeout = UART_TIMEOUT;
    while (DL_UART_isBusy(UART_0_INST)){
        if (--timeout == 0)
            return 0;
    }

    return 1;
}

uint8_t uart_SendBytes(uint8_t *data, uint16_t len){
    uint16_t i;
    uint32_t timeout;

    if (data == NULL || len == 0)
        return 0;

    for (i = 0; i < len; i++){
        // 等待 TX FIFO 有空闲空间
        timeout = UART_TIMEOUT;
        while (DL_UART_isTXFIFOFull(UART_0_INST)){
            if (--timeout == 0)
                return 0;
        }
        // 逐字节发送
        DL_UART_transmitData(UART_0_INST, data[i]);
    }

    // 等待全部数据发送完成
    timeout = UART_TIMEOUT;
    while (DL_UART_isBusy(UART_0_INST)){
        if (--timeout == 0)
            return 0;
    }

    return 1;
}

void uart_Send16Bit(uint16_t data){
    uart_SendByte((uint8_t)(data >> 8));   // 高8位
    uart_SendByte((uint8_t)(data & 0xFF)); // 低8位
}

void uart_Send16Bits(uint16_t *data, uint16_t len){
    if (data == NULL || len == 0)
        return;

    for (uint16_t i = 0; i < len; i++){
        uart_Send16Bit(data[i]); // 复用单次发送函数（默认大端模式）
    }
}

uint8_t uart_SendString(const char *str)
{
    if (str == NULL)
        return 0;

    while (*str)
    {
        if (!uart_SendByte((uint8_t)*str))
            return 0;

        str++;
    }

    return 1;
}
