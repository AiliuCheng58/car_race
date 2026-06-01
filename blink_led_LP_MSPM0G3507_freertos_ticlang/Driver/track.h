#ifndef TRACK_H
#define TRACK_H

#include "Driver/track.h"
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/cdefs.h>

extern volatile uint16_t TRACK_THRESHOLD[8];
extern volatile uint8_t  track;
extern volatile uint16_t adc0[4];
extern volatile uint16_t adc1[4];
extern volatile bool     ready_0;
extern volatile bool     ready_1;

void setTrackBit(uint8_t bit, uint16_t adc, uint8_t i);

#endif