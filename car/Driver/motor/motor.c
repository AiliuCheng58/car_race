#include "motor.h"

#include "ti_msp_dl_config.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/driverlib/dl_timer.h"

#include <stdint.h>

static uint32_t motor_output_to_pwm(float output)
{
    if (output < 0.0f) { // PID 输出为负表示反转，但 PWM 占空比只能用正数
        output = -output; // 取绝对值作为 PWM 大小
    }
    if (output > (float) MOTOR_PWM_MAX) { // 超过最大占空比时限幅
        output = (float) MOTOR_PWM_MAX; // 压到 100% PWM
    }

    return (uint32_t) output; // 转成定时器比较值
}

static void motor_set(GPIO_Regs *port, uint32_t in1_pin, uint32_t in2_pin,
    uint32_t compare_idx, float output)
{
    uint32_t pwm = motor_output_to_pwm(output); // 根据 PID 输出得到 PWM 比较值

    if (output > 0.0f) { // 正输出：按当前接线定义为电机正转
        DL_GPIO_setPins(port, in1_pin); // IN1=1
        DL_GPIO_clearPins(port, in2_pin); // IN2=0
    } else if (output < 0.0f) { // 负输出：按当前接线定义为电机反转
        DL_GPIO_clearPins(port, in1_pin); // IN1=0
        DL_GPIO_setPins(port, in2_pin); // IN2=1
    } else {
        DL_GPIO_clearPins(port, in1_pin); // IN1=0
        DL_GPIO_clearPins(port, in2_pin); // IN2=0，TB6612 该通道停止输出
        pwm = 0U; // 停车时 PWM 也清零
    }

    DL_Timer_setCaptureCompareValue(PWM_0_INST, pwm, compare_idx); // 写入对应 PWM 通道
}

void motor_init(void)
{
    DL_GPIO_setPins(MOTOR_ENABLE_PORT, MOTOR_ENABLE_STBY_PIN); // STBY 拉高，TB6612 脱离待机

    DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN1_PIN); // 左轮 IN1 初始拉低
    DL_GPIO_clearPins(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN2_PIN); // 左轮 IN2 初始拉低
    DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN1_PIN); // 右轮 IN1 初始拉低
    DL_GPIO_clearPins(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN2_PIN); // 右轮 IN2 初始拉低

    DL_Timer_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C0_IDX); // 左轮 PWMA 初始占空比为 0
    DL_Timer_setCaptureCompareValue(PWM_0_INST, 0, GPIO_PWM_0_C1_IDX); // 右轮 PWMB 初始占空比为 0
}

void motor_left_set(float output)
{
    motor_set(MOTOR_LEFT_PORT, MOTOR_LEFT_LEFT_IN1_PIN,
        MOTOR_LEFT_LEFT_IN2_PIN, GPIO_PWM_0_C0_IDX,
        output * MOTOR_LEFT_FORWARD_SIGN); // 左轮使用 PWM_0 通道 C0，方向可用宏校准
}

void motor_right_set(float output)
{
    motor_set(MOTOR_RIGHT_PORT, MOTOR_RIGHT_RIGHT_IN1_PIN,
        MOTOR_RIGHT_RIGHT_IN2_PIN, GPIO_PWM_0_C1_IDX,
        output * MOTOR_RIGHT_FORWARD_SIGN); // 右轮使用 PWM_0 通道 C1，方向可用宏校准
}
