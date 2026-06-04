#ifndef __2TASK_H_
#define __2TASK_H_

#include <stdint.h>

#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"

#include "control.h"
#include "uart.h"
#include "encode.h"
#include "projdefs.h"

extern SemaphoreHandle_t          button_sem;
extern volatile uint8_t           button_task;

void Line_Follow(void *pvParameters);
void OLED(void *pvParameters);
void KEY(void* pvParameters);

#endif
