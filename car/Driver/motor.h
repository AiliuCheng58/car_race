#ifndef __MOTOR_H_
#define __MOTOR_H_

#include "head.h"

#define RPM_ROUND    500.0f
#define PID_DT       0.01f

void motor_init();
float speed_cal(int encoder_cnt);
void motor_left_set(float output, short direction);
void motor_right_set(float output, short direction);
void motor_stop();

#endif