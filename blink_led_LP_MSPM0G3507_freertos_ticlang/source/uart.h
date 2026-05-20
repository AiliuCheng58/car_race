#ifndef __UART_H__
#define __UART_H__

#include "head.h"

#define UART_TIMEOUT 100000

uint8_t uart_SendByte(uint8_t data);
uint8_t uart_SendBytes(uint8_t *data, uint16_t len);
void uart_Send16Bit(uint16_t data);
void uart_Send16Bits(uint16_t *data, uint16_t len);
uint8_t uart_SendString(const char *str);

#endif