#include "motor.h"

#include "ti/driverlib/dl_gpio.h"
#include "ti/driverlib/dl_timer.h"

/* PID 输出可能超过 PWM 范围，这里统一取绝对值并限幅。 */
static uint32_t motor_output_to_pwm(float output)
{
    if (output < 0.0f) {
        output = -output;
    }
    if (output > (float) MOTOR_PWM_MAX) {
        output = (float) MOTOR_PWM_MAX;
    }

    return (uint32_t) output;
}

void motor_init(void)
{
    /*
     * TB6612 的 STBY 必须为高电平才会工作。
     * 方向脚和 PWM 先清零，避免上电瞬间电机误转。
     */
    DL_GPIO_setPins(MOTOR_ENABLE_PORT, MOTOR_ENABLE_STBY_PIN);

    DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN1_PIN);
    DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN2_PIN);
    DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN1_PIN);
    DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN2_PIN);

    DL_Timer_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C0_IDX);
    DL_Timer_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C1_IDX);
}

void motor_left_set(float output, int8_t direction)
{
    uint32_t pwm = motor_output_to_pwm(output);

    /*
     * direction > 0：正转
     * direction < 0：反转
     * direction = 0：停止，两个方向脚都拉低
     */
    if (direction > 0) {
        DL_GPIO_setPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN2_PIN);
    } else if (direction < 0) {
        DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN1_PIN);
        DL_GPIO_setPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN2_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN2_PIN);
        pwm = 0U;
    }

    DL_Timer_setCaptureCompareValue(PWM_0_INST, pwm, GPIO_PWM_0_C0_IDX);
}

void motor_right_set(float output, int8_t direction)
{
    uint32_t pwm = motor_output_to_pwm(output);

    if (direction > 0) {
        DL_GPIO_setPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN2_PIN);
    } else if (direction < 0) {
        DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN1_PIN);
        DL_GPIO_setPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN2_PIN);
    } else {
        DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN1_PIN);
        DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN2_PIN);
        pwm = 0U;
    }

    DL_Timer_setCaptureCompareValue(PWM_0_INST, pwm, GPIO_PWM_0_C1_IDX);
}

void motor_stop(void)
{
    DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN1_PIN);
    DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN2_PIN);
    DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN1_PIN);
    DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN2_PIN);

    DL_Timer_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C0_IDX);
    DL_Timer_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C1_IDX);
}
