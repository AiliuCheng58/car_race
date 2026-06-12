#ifndef __TRACK_H_
#define __TRACK_H_ // 防止 track.h 被重复包含

#include <stdint.h>

typedef struct {
    uint8_t raw;                   /* 循迹模块原始 8 位数据，1 表示压到黑线 */
    int8_t error;                  /* 线相对车身中心的偏差，左负右正 */
    uint8_t valid;                 /* 当前是否有可用于控制的偏差 */
} TrackInfo;

uint8_t Track_receive(void); // 从 UART 缓存里解析循迹模块数据帧，收到新帧返回 1
TrackInfo Track_get_info(void); // 获取最新 raw 和循迹偏差

#endif
