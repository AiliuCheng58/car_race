#ifndef BUTTON_H
#define BUTTON_H

#include <stdint.h>

#include "FreeRTOS.h"
#include "semphr.h"

void Button_init(void);
void Button_switch(void);
uint8_t Button_is_pressed(void);

#endif
