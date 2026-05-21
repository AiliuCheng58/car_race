#ifndef __PID_H_
#define __PID_H_

#include "head.h"

enum pid_mode{
    SPEED_MODE,
    POSITION_MODE
};

typedef struct PID{
    float prev_err;
    float last_prev_err;
    float integral;

    float kp;
    float ki;
    float kd;

    int8_t direction;

    float target;

    int16_t pwm_max;
    int16_t integral_max;

    float output;

    enum pid_mode mode;
}PID;

extern PID pid_left;
extern PID pid_right;

void pid_init(PID *pid, enum pid_mode mode, float target, uint16_t max, float kp, float ki, float kd);
float pid_cal(PID *pid, float cur);
void pid_set();
void pid_display();
int pid_set_mode(PID *pid, enum pid_mode mode);

#endif