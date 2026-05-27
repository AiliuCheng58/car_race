#include "encode.h"

#include "ti/driverlib/dl_gpio.h"

static volatile int32_t encoder_left_count = 0;
static volatile int32_t encoder_right_count = 0;
static uint8_t encoder_left_last_state = 0;
static uint8_t encoder_right_last_state = 0;

/* 读取 AB 两相电平，并压成 0~3 的状态值：bit0=A，bit1=B。 */
static uint8_t encoder_read_state(GPIO_Regs *port, uint32_t a_pin, uint32_t b_pin)
{
    uint32_t pins = DL_GPIO_readPins(port, a_pin | b_pin);
    uint8_t state = 0;

    if ((pins & a_pin) != 0U) {
        state |= 0x01U;
    }
    if ((pins & b_pin) != 0U) {
        state |= 0x02U;
    }

    return state;
}

/*
 * 四倍频正交解码表。
 * 相邻状态变化计为 +/-1；抖动或 00->11 这类跳变计为 0。
 */
// 10 01 == 1000 | 0001 == 1001 == 9
static int8_t encoder_decode_delta(uint8_t old_state, uint8_t new_state)
{
    static const int8_t table[16] = {
         0,  1, -1,  0,
        -1,  0,  0,  1,
         1,  0,  0, -1,
         0, -1,  1,  0
    };

    return table[((old_state & 0x03U) << 2) | (new_state & 0x03U)];
}

static void encoder_update_left(void)
{
    uint8_t state = encoder_read_state(
        ENCODER_LEFT_A_PORT, ENCODER_LEFT_A_PIN, ENCODER_LEFT_B_PIN);
    int8_t delta = encoder_decode_delta(encoder_left_last_state, state);

    encoder_left_count += ((int32_t) delta * ENCODER_LEFT_DIR);
    encoder_left_last_state = state;
}

static void encoder_update_right(void)
{
    uint8_t state = encoder_read_state(
        ENCODER_RIGHT_A_PORT, ENCODER_RIGHT_A_PIN, ENCODER_RIGHT_B_PIN);
    int8_t delta = encoder_decode_delta(encoder_right_last_state, state);

    encoder_right_count += ((int32_t) delta * ENCODER_RIGHT_DIR);
    encoder_right_last_state = state;
}

void encoder_init(void)
{
    /*
     * 输入模式、上拉、双边沿触发和 GPIO 中断使能由 SysConfig 生成。
     * 这里仅记录初始 AB 状态，并打开 NVIC 里的 GPIOA/GPIOB 中断。
     */
    encoder_left_last_state = encoder_read_state(
        ENCODER_LEFT_A_PORT, ENCODER_LEFT_A_PIN, ENCODER_LEFT_B_PIN);
    encoder_right_last_state = encoder_read_state(
        ENCODER_RIGHT_A_PORT, ENCODER_RIGHT_A_PIN, ENCODER_RIGHT_B_PIN);
    encoder_clear();

    DL_GPIO_clearInterruptStatus(ENCODER_LEFT_A_PORT, ENCODER_LEFT_PIN_MASK);
    DL_GPIO_clearInterruptStatus(ENCODER_RIGHT_A_PORT, ENCODER_RIGHT_PIN_MASK);

    NVIC_ClearPendingIRQ(ENCODER_LEFT_INT_IRQN);
    NVIC_ClearPendingIRQ(ENCODER_RIGHT_INT_IRQN);
    NVIC_EnableIRQ(ENCODER_LEFT_INT_IRQN);
    NVIC_EnableIRQ(ENCODER_RIGHT_INT_IRQN);
}

void encoder_clear(void)
{
    uint32_t primask = __get_PRIMASK();

    __disable_irq();
    encoder_left_count = 0;
    encoder_right_count = 0;
    __set_PRIMASK(primask);
}

int32_t encoder_get_left_count(void)
{
    return encoder_left_count;
}

int32_t encoder_get_right_count(void)
{
    return encoder_right_count;
}

EncoderSample encoder_get_and_clear(void)
{
    EncoderSample sample;
    uint32_t primask = __get_PRIMASK();

    /* PID 定时器读取时短暂关中断，避免读数和清零被编码器中断打断。 */
    __disable_irq();
    sample.left = encoder_left_count;
    sample.right = encoder_right_count;
    encoder_left_count = 0;
    encoder_right_count = 0;
    __set_PRIMASK(primask);

    return sample;
}

float encoder_count_to_rpm(int32_t count)
{
    /*
     * RPM = 一个采样周期内的计数 / 每圈计数 / 采样周期秒数 * 60
     * 例如 10 ms 内数到 50 个计数，每圈 500 计数：
     * 50 / 500 / 0.01 * 60 = 600 RPM
     */
    return ((float) count / ENCODER_COUNTS_PER_REV)
        * (60.0f / ENCODER_SAMPLE_PERIOD_S);
}

/*
 * GPIOA/GPIOB 都走 GROUP1_IRQHandler。
 * 先取出并清除已触发的边沿，再读取当前 AB 状态做一次解码。
 */
void GROUP1_IRQHandler(void)
{
    uint32_t left_status = DL_GPIO_getEnabledInterruptStatus(
        ENCODER_LEFT_A_PORT, ENCODER_LEFT_PIN_MASK);
    uint32_t right_status = DL_GPIO_getEnabledInterruptStatus(
        ENCODER_RIGHT_A_PORT, ENCODER_RIGHT_PIN_MASK);

    if (left_status != 0U) {
        DL_GPIO_clearInterruptStatus(ENCODER_LEFT_A_PORT, left_status);
        encoder_update_left();
    }

    if (right_status != 0U) {
        DL_GPIO_clearInterruptStatus(ENCODER_RIGHT_A_PORT, right_status);
        encoder_update_right();
    }
}
