#include "motor.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/driverlib/dl_timer.h"

void motor_init(){
    DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN1_PIN);
    DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN2_PIN);
    DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN1_PIN);
    DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN2_PIN);

    DL_Timer_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C0_IDX);
    DL_Timer_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C1_IDX);
}

float speed_cal(int encoder_cnt){
    float rpm = (float)encoder_cnt
                / RPM_ROUND
                * (60.0f) 
                / PID_DT;
    return rpm;
}

void motor_left_set(float output, short direction){
    if(direction > 0)
    {
        DL_GPIO_setPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN2_PIN);
    }
    else if(direction < 0)
    {
        DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN1_PIN);
        DL_GPIO_setPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN2_PIN);
    }
    else
    {
        DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN2_PIN);

        output = 0;
    }

    DL_Timer_setCaptureCompareValue(
        PWM_0_INST,
        (uint32_t)output,
        GPIO_PWM_0_C0_IDX
    );
}

void motor_right_set(float output, short direction){
    if(direction > 0)
    {
        DL_GPIO_setPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN2_PIN);
    }
    else if(direction < 0)
    {
        DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN1_PIN);
        DL_GPIO_setPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN2_PIN);
    }
    else
    {
        DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN2_PIN);

        output = 0;
    }

    DL_Timer_setCaptureCompareValue(
        PWM_0_INST,
        (uint32_t)output,
        GPIO_PWM_0_C1_IDX
    );
}

void motor_stop(){
    DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN1_PIN);
    DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN2_PIN);
    DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN1_PIN);
    DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN2_PIN);

    DL_Timer_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C0_IDX);
    DL_Timer_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C1_IDX);
}