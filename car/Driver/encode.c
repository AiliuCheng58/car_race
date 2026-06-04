#include "encode.h"

#include "FreeRTOS.h"
#include "semphr.h"
#include "ti_msp_dl_config.h"
#include "ti/driverlib/dl_gpio.h"

extern SemaphoreHandle_t button_sem;

static volatile int32_t encoder_left_count = 0; // 左轮累计编码器计数，中断和 PID 定时器都会访问
static volatile int32_t encoder_right_count = 0; // 右轮累计编码器计数，中断和 PID 定时器都会访问
static uint8_t encoder_left_last_state = 0; // 左轮上一次 AB 相状态，用于判断本次转动方向
static uint8_t encoder_right_last_state = 0; // 右轮上一次 AB 相状态，用于判断本次转动方向

static uint8_t encoder_read_state(GPIO_Regs *port, uint32_t a_pin, uint32_t b_pin)
{
    uint32_t pins = DL_GPIO_readPins(port, a_pin | b_pin); // 一次读出 A/B 两个引脚电平
    uint8_t state = 0U; // bit0 表示 A 相，bit1 表示 B 相

    if ((pins & a_pin) != 0U) { // A 相为高电平
        state |= 0x01U; // 设置 bit0
    }
    if ((pins & b_pin) != 0U) { // B 相为高电平
        state |= 0x02U; // 设置 bit1
    }

    return state; // 返回 0~3，对应 00、01、10、11 四种 AB 状态
}

static int8_t encoder_decode_delta(uint8_t old_state, uint8_t new_state)
{
    static const int8_t table[16] = { // 下标是 old_state<<2 | new_state，值是本次增量
         0,  1, -1,  0,
        -1,  0,  0,  1,
         1,  0,  0, -1,
         0, -1,  1,  0
    }; // 相邻状态变化计为 +/-1，抖动或 00->11 这类跳变计为 0

    return table[((old_state & 0x03U) << 2) | (new_state & 0x03U)]; // 合法相邻跳变返回 +/-1
}

void encoder_init(void)
{
    uint32_t primask = __get_PRIMASK(); // 保存原中断状态

    __disable_irq(); // 初始化状态和清零计数时避免编码器中断打断
    encoder_left_last_state = encoder_read_state(
        ENCODER_LEFT_PORT, ENCODER_LEFT_LEFT_A_PIN, ENCODER_LEFT_LEFT_B_PIN); // 记录左轮上电时的 AB 状态
    encoder_right_last_state = encoder_read_state(
        ENCODER_RIGHT_PORT, ENCODER_RIGHT_RIGHT_A_PIN,
        ENCODER_RIGHT_RIGHT_B_PIN); // 记录右轮上电时的 AB 状态
    encoder_left_count = 0; // 左轮累计计数清零
    encoder_right_count = 0; // 右轮累计计数清零
    __set_PRIMASK(primask); // 恢复原中断状态

    DL_GPIO_clearInterruptStatus(ENCODER_LEFT_PORT,
        ENCODER_LEFT_LEFT_A_PIN | ENCODER_LEFT_LEFT_B_PIN); // 清左轮 AB 相残留中断标志
    DL_GPIO_clearInterruptStatus(ENCODER_RIGHT_PORT,
        ENCODER_RIGHT_RIGHT_A_PIN | ENCODER_RIGHT_RIGHT_B_PIN); // 清右轮 AB 相残留中断标志

    NVIC_ClearPendingIRQ(ENCODER_LEFT_INT_IRQN); // 清左轮所在 GPIO 组的 NVIC 挂起位
    NVIC_ClearPendingIRQ(GPIO_MULTIPLE_GPIOB_INT_IRQN); // 清 GPIOB 组的 NVIC 挂起位
    NVIC_EnableIRQ(ENCODER_LEFT_INT_IRQN); // 打开左轮编码器 GPIO 中断
    NVIC_EnableIRQ(GPIO_MULTIPLE_GPIOB_INT_IRQN); // 打开右轮编码器和按键所在 GPIOB 中断
}

void GROUP1_IRQHandler(void)
{
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint32_t left_status = DL_GPIO_getEnabledInterruptStatus(ENCODER_LEFT_PORT,
        ENCODER_LEFT_LEFT_A_PIN | ENCODER_LEFT_LEFT_B_PIN); // 左轮 AB 相触发状态
    uint32_t right_status = DL_GPIO_getEnabledInterruptStatus(ENCODER_RIGHT_PORT,
        ENCODER_RIGHT_RIGHT_A_PIN | ENCODER_RIGHT_RIGHT_B_PIN); // 右轮 AB 相触发状态
    uint32_t key_status = DL_GPIO_getEnabledInterruptStatus(KEY_PORT,
        KEY_PIN_21_PIN); // 按键中断触发状态

    if (left_status != 0U) { // 左轮 A/B 任意一相发生边沿变化
        DL_GPIO_clearInterruptStatus(ENCODER_LEFT_PORT, left_status); // 先清左轮中断标志
        uint8_t state = encoder_read_state(
            ENCODER_LEFT_PORT, ENCODER_LEFT_LEFT_A_PIN, ENCODER_LEFT_LEFT_B_PIN); // 读取左轮当前 AB 状态
        int8_t delta = encoder_decode_delta(encoder_left_last_state, state); // 根据新旧状态算本次增量

        encoder_left_count += ((int32_t) delta * ENCODER_LEFT_DIR); // 乘方向系数后累计到左轮计数
        encoder_left_last_state = state; // 保存本次状态，下一次解码要用
    }

    if (right_status != 0U) { // 右轮 A/B 任意一相发生边沿变化
        DL_GPIO_clearInterruptStatus(ENCODER_RIGHT_PORT, right_status); // 先清右轮中断标志
        uint8_t state = encoder_read_state(
            ENCODER_RIGHT_PORT, ENCODER_RIGHT_RIGHT_A_PIN,
            ENCODER_RIGHT_RIGHT_B_PIN); // 读取右轮当前 AB 状态
        int8_t delta = encoder_decode_delta(encoder_right_last_state, state); // 根据新旧状态算本次增量

        encoder_right_count += ((int32_t) delta * ENCODER_RIGHT_DIR); // 乘方向系数后累计到右轮计数
        encoder_right_last_state = state; // 保存本次状态，下一次解码要用
    }

    if (key_status != 0U) { // 按键按下
        DL_GPIO_clearInterruptStatus(KEY_PORT, key_status); // 清除按键中断标志
        if (button_sem != NULL) {
            xSemaphoreGiveFromISR(button_sem, &xHigherPriorityTaskWoken);
        }
    }

    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

EncoderSample encoder_get_and_clear(void)
{
    EncoderSample sample;                           // 保存一个 PID 周期内的左右轮计数
    uint32_t primask = __get_PRIMASK();             // 保存原中断状态

    __disable_irq();                                // 读取并清零时避免编码器中断打断
    sample.left = encoder_left_count;               // 保存左轮本周期计数
    sample.right = encoder_right_count;             // 保存右轮本周期计数
    encoder_left_count = 0;                         // 左轮计数清零，开始下一个 PID 周期
    encoder_right_count = 0;                        // 右轮计数清零，开始下一个 PID 周期
    __set_PRIMASK(primask);                         // 恢复原中断状态

    return sample;                                  // 返回刚刚保存的左右轮增量
}

float encoder_left_count_to_rpm(int32_t count)
{
    return ((float) count / ENCODER_LEFT_COUNTS_PER_REV)
        * (60.0f / ENCODER_SAMPLE_PERIOD_S); // 左轮计数换算成 RPM
}

float encoder_right_count_to_rpm(int32_t count)
{
    return ((float) count / ENCODER_RIGHT_COUNTS_PER_REV)
        * (60.0f / ENCODER_SAMPLE_PERIOD_S); // 右轮计数换算成 RPM
}
