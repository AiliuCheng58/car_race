#ifndef __2TASK_H_
#define __2TASK_H_

#include "head.h"

#include "control.h"
#include "uart.h"

void Line_Follow(void *pvParameters);
void UART(void *pvParameters);

#endif