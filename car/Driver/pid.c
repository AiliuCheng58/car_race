#include "pid.h"
#include "motor.h"

PID pid_left;
PID pid_right;

void pid_init(PID *pid, enum pid_mode mode, float target, uint16_t max, float kp, float ki, float kd)
{
    if(pid == NULL)
        return;
    pid->target = target;
    pid->pwm_max = max;
    pid->integral_max = max / 2;

    pid->kp = kp;
    pid->ki = ki;
    pid->kd = kd;

    pid->integral = 0.0f;
    pid->prev_err = 0.0f;
    pid->last_prev_err = 0.0f;
    pid->output = 0.0f;

    pid->direction = 0;
    pid->mode = mode;
}

float pid_cal(PID *pid, float cur)
{
    if (pid == NULL)
        return POINTER_ERROR;

    float err = pid->target - cur;
    float delta_err = err - pid->prev_err;
    float delta_output = 0.0f;

    switch (pid->mode){
        case POSITION_MODE:
            pid->integral += err;

            if (pid->integral > (float)pid->integral_max)
                pid->integral = (float)pid->integral_max;
            else if (pid->integral < -(float)pid->integral_max)
                pid->integral = -(float)pid->integral_max;

            pid->output = pid->kp * err
                        + pid->ki * pid->integral
                        + pid->kd * delta_err;
            break;

        case SPEED_MODE:
            delta_output = pid->kp * delta_err
                        + pid->ki * err
                        + pid->kd * (err - 2.0f * pid->prev_err + pid->last_prev_err);
            pid->output += delta_output;
            break;

        default:
            return NORMAL_ERROR;
    }
    if (pid->output > 0)
        pid->direction = 1;
    else if (pid->output < 0)
        pid->direction = -1;
    else
        pid->direction = 0;

    if (pid->output > (float)pid->pwm_max)
        pid->output = (float)pid->pwm_max;
    else if(pid->output < -(float)pid->pwm_max)
        pid->output = -(float)pid->pwm_max;

    pid->last_prev_err = pid->prev_err;
    pid->prev_err = err;

    return fabsf(pid->output);
}

void pid_set(){
    //通过串口接收数据进行pid调参
}

void pid_display(){
    //显示在oled屏幕上
}

int pid_set_mode(PID *pid, enum pid_mode mode){
    if(pid == NULL)
        return POINTER_ERROR;
    if(mode == SPEED_MODE || mode == POSITION_MODE){
        pid->mode = mode;
        return OK;
    }
    return NORMAL_ERROR;
}

void TIMER_0_INST_IRQHandler(void){
    //获取编码器数据
    //先设成个定值吧
    int left_cnt = 100;
    int right_cnt = 120;

    float left_cur = speed_cal(left_cnt);
    float right_cur = speed_cal(right_cnt);

    float left_pwm = pid_cal(&pid_left, left_cur);
    motor_left_set(left_pwm, pid_left.direction);
    float right_pwm = pid_cal(&pid_right, right_cur);
    motor_right_set(right_pwm, pid_right.direction);

    DL_Timer_clearInterruptStatus(TIMER_0_INST, DL_TIMERA_INTERRUPT_ZERO_EVENT);
}