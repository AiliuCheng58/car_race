#include "track.h"
#include "uart.h"
#include "ti_msp_dl_config.h"

static volatile uint8_t raw = 0x00U;                    // 最新循迹原始值，上电还没收到帧时先按丢线处理
static volatile uint8_t raw_flag = 0U;                  // 是否已经收到过循迹模块的完整帧
static int8_t last_error = 0;                           // 上一次有效偏差，丢线时用于沿原方向找线
static uint8_t error_valid = 0U;                        // 是否已经得到过至少一次可用偏差

static uint8_t Track_line(uint8_t raw, int8_t *error)
{
    // bit0 在最左，bit7 在最右，左负右正
    static const int8_t weight[8] = {
        -14, -10, -6, -3, 3, 6, 10, 14
    };
    int16_t sum = 0;
    uint8_t count = 0U;
    if (error == NULL)
        return 0U;
    if (raw == 0U)
        return 0U;   // 没有任何探头压线，沿用上一次有效偏差

    for (uint8_t i = 0U; i < 8U; i++) {
        if ((raw & (1U << i)) != 0U) {
            sum += weight[i];
            count++;
        }
    }
    if (count == 0U){
        *error = last_error;
        return 0U;
    }
    *error = (int8_t)sum;
    return 1U;
}

uint8_t Track_receive(void)                                                     //循迹帧格式：0xFE raw 0xEF
{
    uint32_t primask = __get_PRIMASK();                                         // 保存原中断状态
    uint8_t result = 0U;                                                        // 本次调用是否解析到至少一帧循迹数据

    __disable_irq();                                                            // 原子操作

    while (RX_index >= 3U) {                                                    // 至少 3 字节才可能组成 0xFE/raw/0xEF
        uint16_t i;
        uint8_t flag = 0U;                                                      // 本轮是否找到完整循迹帧

        for (i = 0U; i <= (RX_index - 3U); i++) {                               // 在缓存里滑动查找帧头帧尾
            if ((UART_Data[i] == 0xFEU) && (UART_Data[i + 2U] == 0xEFU)) {      // 找到完整三字节循迹帧
                raw = UART_Data[i + 1U];                                        // 中间字节就是循迹模块原始状态
                raw_flag = 1U;                                                  // 标记至少收到过一帧循迹数据
                result = 1U;                                                    // 告诉控制层本轮有新循迹数据
                UART_MoveBytes(i + 3U);                                         // 消费掉这一帧以及帧前无效数据
                flag = 1U;                                                      // 标记本轮已经成功解析到一帧
                break;                                                          // 本次只处理最新找到的一帧
            }
        }
        if (!flag) {                                                            // 当前收到半包，丢弃开头无效数据并非阻塞等待剩下半包
            if (UART_Data[RX_index - 2U] == 0xFEU) {
                UART_Data[0] = UART_Data[RX_index - 2U];                        // 保留末尾可能尚未收完整的帧头
                UART_Data[1] = UART_Data[RX_index - 1U];                        // 保留帧头后的 raw，等待下一次收到帧尾
                RX_index = 2U;                                                  // 缓存里只留下 0xFE/raw
            } else if (UART_Data[RX_index - 1U] == 0xFEU) {
                UART_Data[0] = 0xFEU;                                           // 保留可能的新帧头
                RX_index = 1U;                                                  // 缓存里只剩这个帧头
            } else {
                RX_index = 0U;                                                  // 没有可用帧头，直接丢弃无效数据
            }
            break;
        }
    }
    __set_PRIMASK(primask);                                                     // 恢复原来的中断状态
    return result;                                                               // 返回本次是否收到新循迹帧
}

TrackInfo Track_get_info(void)
{
    TrackInfo info;                                                             // 返回给控制层的循迹结果

    info.raw = raw;                                                             // 保存循迹模块原始 8 位状态，1 表示对应探头压到黑线
    info.valid = 0U;                                                            // 默认没有可用偏差，只有解析成功或能沿用历史偏差时才置 1

    if (raw_flag == 0U) {                                                       // 还没收到过循迹帧
        info.error = 0;                                                         // 不给任何方向偏差
        return info;
    }

    if (Track_line(info.raw, &info.error) == 0U) {
        if (error_valid == 0U) {                                                // 上电后还没识别到过黑线
            info.error = 0;                                                     // 不给默认方向，控制层会停车等真正偏差
            return info;
        }

        info.error = last_error;                                                // 未识别或 raw=0 时沿用上一次偏差
        info.valid = 1U;                                                        // 有历史偏差可用，继续按普通循迹控制
        return info;                                                            // 沿用历史偏差时不更新 last_error
    }

    last_error = info.error;                                                    // 记录最近一次有效偏差
    error_valid = 1U;                                                           // 已经至少识别到过一次黑线位置
    info.valid = 1U;                                                            // 本次偏差可以直接用于控制

    return info;                                                                // 返回本次循迹结果
}
