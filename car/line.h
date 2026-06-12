#ifndef __LINE_H_
#define __LINE_H_ // 防止 line.h 被重复包含

#include <stdint.h>

#include "track/track.h"

#define LINE_FOLLOW_PERIOD_MS  (5U)    // 循迹任务刷新周期，单位 ms
#define LINE_FOLLOW_TIMEOUT_MS (500U)  // 超过这个时间没有新循迹帧就认为丢线

typedef struct {
    TrackInfo info;     // 当前周期循迹数据
    uint32_t age_ms;    // 距离上一帧循迹数据的时间
    uint8_t on_line;    // 当前周期 raw 是否看到黑线
    uint8_t last_line;  // 上一周期 raw 是否看到黑线
} LineTrack;

void Line_track_reset(LineTrack *track);              // 清空循迹快照和超时状态
void Line_track_read(LineTrack *track);               // 读取一周期循迹数据并更新边沿状态
TrackInfo Line_track_info(void);                      // 获取给遥测显示使用的循迹快照
uint8_t Line_track_lost(const LineTrack *track);      // 判断循迹串口是否超时
uint8_t Line_track_rise(const LineTrack *track);      // 判断是否从白区进入黑线
uint8_t Line_track_fall(const LineTrack *track);      // 判断是否从黑线离开到白区

#endif
