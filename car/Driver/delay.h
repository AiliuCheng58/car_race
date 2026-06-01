#ifndef __DELAY_H_
#define __DELAY_H_ // 防止 delay.h 被重复包含

#include <stdint.h>

void delay_ms(uint32_t ms); // 阻塞延时，单位 ms

#endif
