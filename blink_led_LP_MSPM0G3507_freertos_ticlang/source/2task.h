#ifndef __2TASK_H_
#define __2TASK_H_

#include "head.h"

void UART(void *pvParameters);
void LED(void *pvParameters);
void INIT(void *pvParameters);
void ICM(void *pvParameters);

#endif