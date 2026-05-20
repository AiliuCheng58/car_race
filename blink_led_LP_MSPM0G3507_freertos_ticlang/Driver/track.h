#ifndef TRACK_H
#define TRACK_H

#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>

#define TRACK_THRESHOLD (3000)

extern volatile uint8_t  track;
extern volatile uint16_t adc0[4];
extern volatile uint16_t adc1[4];
extern volatile bool     ready_0;
extern volatile bool     ready_1;

void setTrackBit(uint8_t bit, uint16_t adc);

#endif