#ifndef __UART_H__
#define __UART_H__ // 防止 uart.h 被重复包含

#include <stdint.h>
#include "ti_msp_dl_config.h"

#include <stdbool.h>
#include <stddef.h>
#include <string.h>

#define UART_RX_BUFFER_SIZE 256 // UART 接收缓存大小，当前只用于保存循迹模块数据

void UART_SendString(char *str); // 串口阻塞发送字符串
void UART_init(void); // 清中断标志并打开 UART 接收中断
void UART_MoveBytes(uint16_t count); // 从接收缓存头部移除已处理的字节

extern volatile uint8_t UART_Data[UART_RX_BUFFER_SIZE]; // UART 接收缓存数组
extern volatile uint16_t RX_index; // UART 缓存当前有效长度

#endif
