#ifndef __PID_INTERNAL_H_
#define __PID_INTERNAL_H_

#include "pid.h"

void pid_write_params(PID *pid, uint8_t has_kp, float kp,
    uint8_t has_ki, float ki, uint8_t has_kd, float kd);

#endif
