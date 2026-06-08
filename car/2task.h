#ifndef __2TASK_H_
#define __2TASK_H_

#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

#define BUTTON_TASK_IDLE        (0U) // 空闲模式，上电默认停在这里
#define BUTTON_TASK_LINE_FOLLOW (1U) // 循迹模式，按键第一次切到这里
#define BUTTON_TASK_MODE_COUNT  (4U) // 预留后续任务模式数量

extern SemaphoreHandle_t          button_sem;
extern volatile uint8_t           button_task;

void Line_Follow(void *pvParameters);
void OLED(void *pvParameters);
void KEY(void* pvParameters);

#endif
