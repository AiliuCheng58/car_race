#include "line.h"

#include "FreeRTOS.h"
#include "task.h"

static volatile TrackInfo line_track_info; // 循迹任务保存给遥测显示任务使用

static void Line_track_show(const TrackInfo *info) // 更新遥测显示用的循迹快照
{
    taskENTER_CRITICAL();                // 保护多任务共享快照
    line_track_info.raw = info->raw;     // 保存原始 8 位循迹状态
    line_track_info.error = info->error; // 保存本周期循迹偏差
    line_track_info.valid = info->valid; // 保存偏差是否可用
    taskEXIT_CRITICAL();                 // 快照更新完成
}

void Line_track_reset(LineTrack *track) // 清空当前循迹状态
{
    track->info.raw = 0U;                 // raw 清零，表示当前没有看到黑线
    track->info.error = 0;                // 偏差清零，避免旧偏差参与控制
    track->info.valid = 0U;               // 标记当前偏差不可用
    track->age_ms = LINE_FOLLOW_TIMEOUT_MS; // 上电或退出模式后先按超时处理
    track->on_line = 0U;                  // 当前周期不在线上
    track->last_line = 0U;                // 上一周期也按不在线上处理
    Line_track_show(&track->info);        // 同步清空 OLED 遥测快照
}

void Line_track_read(LineTrack *track) // 读取循迹模块数据并记录边沿
{
    track->last_line = track->on_line;  // 先保存上一周期是否在线上

    if (Track_receive() != 0U)          // 收到新帧
        track->age_ms = 0U;             // 超时计数清零
    else if (track->age_ms < LINE_FOLLOW_TIMEOUT_MS) // 没收到新帧且还没到超时上限
        track->age_ms += LINE_FOLLOW_PERIOD_MS;      // 累加等待时间

    track->info = Track_get_info();                       // 获取最新 raw 和偏差
    track->on_line = (track->info.raw != 0U) ? 1U : 0U;    // raw 非 0 表示当前看到黑线
    Line_track_show(&track->info);                         // 同步给 OLED 遥测
}

TrackInfo Line_track_info(void) // 读取遥测显示用快照
{
    TrackInfo info; // 保存临界区内复制出的循迹状态

    taskENTER_CRITICAL();                // 读取多任务共享快照
    info.raw = line_track_info.raw;      // 复制 raw
    info.error = line_track_info.error;  // 复制偏差
    info.valid = line_track_info.valid;  // 复制有效标志
    taskEXIT_CRITICAL();                 // 快照读取完成

    return info;                         // 返回本次复制出的稳定快照
}

uint8_t Line_track_lost(const LineTrack *track) // 判断循迹串口是否已经超时
{
    return track->age_ms >= LINE_FOLLOW_TIMEOUT_MS;
}

uint8_t Line_track_rise(const LineTrack *track) // 判断白区进入黑线
{
    return (track->on_line != 0U) && (track->last_line == 0U);
}

uint8_t Line_track_fall(const LineTrack *track) // 判断黑线离开到白区
{
    return (track->on_line == 0U) && (track->last_line != 0U);
}
