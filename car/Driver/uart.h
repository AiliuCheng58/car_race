#ifndef __UART_H__
#define __UART_H__
#include "head.h"
#define UART_RX_BUFFER_SIZE 256
void uart0_send_char(char ch);
void uart0_send_string(char *str);
void uart_init(void);

extern SemaphoreHandle_t UART_xSemaphore;
extern volatile uint8_t uart_data[UART_RX_BUFFER_SIZE];
extern volatile uint16_t uart_rx_index;

#endif