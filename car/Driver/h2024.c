#include "h2024.h"

#include "control.h"
#include "mpu6050.h"
#include "track.h"
#include "ti_msp_dl_config.h"

#include "FreeRTOS.h"
#include "task.h"

#include "ti/driverlib/dl_gpio.h"

#include <stddef.h>
#include <stdint.h>

#define H2024_LOOP_MS                    (5U) // H 题任务状态机刷新周期
#define H2024_IMU_PERIOD_MS              (10U) // MPU6050 航向积分周期
#define H2024_TRACK_TIMEOUT_MS           (500U) // 弧线阶段超过该时间没有新 raw 就停车
#define H2024_ARC_ACQUIRE_MS             (450U) // 刚进入弧线时允许低速前进一小段时间寻找黑线
#define H2024_BUTTON_DEBOUNCE_COUNT      (4U) // 按键状态连续稳定 4 次才确认，约 20ms
#define H2024_GYRO_CALIBRATION_SAMPLES   (200U) // 上电静止时采集 200 次，计算陀螺仪零偏

#define H2024_STRAIGHT_AB_CD_CM          (100.0f) // 题面中 AB、CD 两段水平直线长度
#define H2024_DIAGONAL_AC_BD_CM          (128.1f) // sqrt(100^2 + 80^2)，题面中 AC、BD 斜线长度
#define H2024_ARC_CM                     (125.7f) // pi * 40cm，题面中单个半圆弧长度
#define H2024_HEADING_AB_DEG             (0.0f) // 按 START 前令车头朝向 B，并把该方向记作 0 度
#define H2024_HEADING_CD_DEG             (180.0f) // C -> D 与 A -> B 相反，目标航向为 180 度
#define H2024_HEADING_AC_DEG             (38.7f) // A -> C 相对 A -> B 顺时针偏转 atan(80 / 100)
#define H2024_HEADING_BD_DEG             (141.3f) // B -> D 相对 A -> B 顺时针偏转 180 - atan(80 / 100)
#define H2024_GYRO_SIGN                  (1.0f) // 若实测右转时 yaw 变小，把这里改成 -1.0f

#define H2024_STRAIGHT_BASE_RPM          (75.0f) // 无轨直线基础速度
#define H2024_STRAIGHT_YAW_KP            (2.2f) // 直线航向误差每 1 度增加的左右轮速度差
#define H2024_STRAIGHT_MAX_TURN_RPM      (22.0f) // 直线航向修正限幅，保证两轮始终前进
#define H2024_ARC_BASE_RPM               (60.0f) // 半圆弧循迹基础速度
#define H2024_ARC_ACQUIRE_RPM            (35.0f) // 尚未找到弧线黑线时使用的低速前进速度
#define H2024_ARC_KP                     (4.0f) // 弧线循迹比例系数
#define H2024_ARC_KD                     (1.2f) // 弧线循迹微分系数
#define H2024_ARC_MAX_TURN_RPM           (30.0f) // 弧线循迹修正限幅，保证两轮始终前进

#define H2024_POINT_ALERT_MS             (180U) // 每经过一个点时声光提示持续时间
#define H2024_FINISH_ALERT_MS            (800U) // 完成任务后声光提示持续时间
#define H2024_ERROR_ALERT_MS             (1200U) // 传感器异常时声光提示持续时间
typedef enum {
    H2024_SEGMENT_STRAIGHT = 0U, // 无轨直线段：编码器测距，MPU6050 保持航向
    H2024_SEGMENT_ARC // 黑色半圆弧：编码器测距，八路灰度循迹
} H2024SegmentType;

typedef struct {
    H2024SegmentType type; // 本路段使用直线控制还是弧线循迹
    float distance_cm; // 本路段的默认行驶距离
    char end_point; // 完成本路段后到达的点，用于调试理解路线
    float heading_deg; // 直线路段的目标航向，弧线路段不使用该值
} H2024Segment;

typedef struct {
    uint8_t sample; // 最近一次采样到的按键状态
    uint8_t stable; // 已经消抖确认的按键状态
    uint8_t count; // 当前状态连续出现的次数
} H2024Button;

static const H2024Segment task1_segments[] = {
    {H2024_SEGMENT_STRAIGHT, H2024_STRAIGHT_AB_CD_CM, 'B', H2024_HEADING_AB_DEG} // 要求 1：A -> B
};

static const H2024Segment task2_segments[] = {
    {H2024_SEGMENT_STRAIGHT, H2024_STRAIGHT_AB_CD_CM, 'B', H2024_HEADING_AB_DEG}, // 要求 2：A -> B
    {H2024_SEGMENT_ARC, H2024_ARC_CM, 'C', 0.0f}, // B -> C，沿右侧半圆弧
    {H2024_SEGMENT_STRAIGHT, H2024_STRAIGHT_AB_CD_CM, 'D', H2024_HEADING_CD_DEG}, // C -> D
    {H2024_SEGMENT_ARC, H2024_ARC_CM, 'A', 0.0f} // D -> A，沿左侧半圆弧
};

static const H2024Segment task3_segments[] = {
    {H2024_SEGMENT_STRAIGHT, H2024_DIAGONAL_AC_BD_CM, 'C', H2024_HEADING_AC_DEG}, // 要求 3/4：A -> C
    {H2024_SEGMENT_ARC, H2024_ARC_CM, 'B', 0.0f}, // C -> B，沿右侧半圆弧
    {H2024_SEGMENT_STRAIGHT, H2024_DIAGONAL_AC_BD_CM, 'D', H2024_HEADING_BD_DEG}, // B -> D
    {H2024_SEGMENT_ARC, H2024_ARC_CM, 'A', 0.0f} // D -> A，沿左侧半圆弧
};

static volatile H2024Telemetry telemetry; // 串口监视任务读取的 H 题状态快照
static H2024Button mode_button; // MODE 按键消抖状态
static H2024Button start_button; // START 按键消抖状态
static const H2024Segment *segments; // 当前要求对应的路段数组
static uint8_t segment_count = 0U; // 当前要求一圈包含的路段数量
static uint8_t target_laps = 1U; // 当前要求需要完成的圈数
static uint8_t segment_index = 0U; // 当前正在执行的路段序号
static uint8_t current_lap = 1U; // 当前圈数
static uint8_t mission_running = 0U; // 当前是否正在执行路线
static uint32_t track_age_ms = H2024_TRACK_TIMEOUT_MS; // 距离最近一帧 raw 已经过去的时间
static uint32_t signal_ms = 0U; // 蜂鸣器和 LED 剩余提示时间
static uint32_t imu_elapsed_ms = 0U; // 距离上一次 MPU6050 采样已经过去的时间
static uint16_t arc_elapsed_ms = 0U; // 当前弧线路段已运行时间，用于入口找线宽限
static uint8_t arc_line_acquired = 0U; // 当前弧线是否已经实际读到过非零 raw
static float yaw_deg = 0.0f; // 当前相对航向角
static float target_yaw_deg = 0.0f; // 当前直线路段目标航向角
static float gyro_bias_dps = 0.0f; // 静止校准得到的 Z 轴角速度零偏
static int8_t last_track_error = 0; // 上一次弧线循迹偏差，给 D 项使用

static float h2024_limit(float value, float limit)
{
    if (value > limit) return limit; // 超过正向限幅
    if (value < -limit) return -limit; // 超过负向限幅
    return value; // 没有超过限幅
}

static float h2024_wrap_angle(float angle)
{
    while (angle > 180.0f) angle -= 360.0f; // 统一限制到 -180~180 度
    while (angle < -180.0f) angle += 360.0f;
    return angle;
}

static void h2024_set_signal(uint8_t enabled)
{
    if (enabled != 0U) {
        DL_GPIO_setPins(H2024_SIGNAL_PORT, H2024_SIGNAL_BUZZER_PIN); // 有源蜂鸣器高电平鸣叫
        DL_GPIO_clearPins(LED_PORT, LED_PIN_0_PIN); // 板载 LED 低电平点亮
    } else {
        DL_GPIO_clearPins(H2024_SIGNAL_PORT, H2024_SIGNAL_BUZZER_PIN); // 关闭蜂鸣器
        DL_GPIO_setPins(LED_PORT, LED_PIN_0_PIN); // 板载 LED 恢复熄灭
    }
}

static void h2024_alert(uint32_t duration_ms)
{
    signal_ms = duration_ms; // 记录剩余提示时间
    h2024_set_signal(1U); // 立即打开声光提示
}

static void h2024_update_signal(void)
{
    if (signal_ms == 0U) return; // 当前没有正在执行的提示

    if (signal_ms <= H2024_LOOP_MS) {
        signal_ms = 0U; // 本轮结束提示
        h2024_set_signal(0U); // 关闭 LED 和蜂鸣器
    } else {
        signal_ms -= H2024_LOOP_MS; // 扣掉本轮已经过去的时间
    }
}

static uint8_t h2024_button_pressed(H2024Button *button, uint32_t pin)
{
    uint8_t sample = (DL_GPIO_readPins(H2024_BUTTONS_PORT, pin) == 0U) ?
        1U : 0U; // 按键使用上拉输入，按下时读取到低电平

    if (sample == button->sample) {
        if (button->count < H2024_BUTTON_DEBOUNCE_COUNT) button->count++; // 状态持续稳定
    } else {
        button->sample = sample; // 发现变化后从第一次采样重新计数
        button->count = 1U;
    }

    if ((button->count >= H2024_BUTTON_DEBOUNCE_COUNT) &&
        (button->stable != sample)) {
        button->stable = sample; // 消抖完成，更新稳定状态
        return sample; // 只在确认按下的瞬间返回 1，松开不会触发动作
    }

    return 0U;
}

static void h2024_update_imu(void)
{
    float gyro_z_dps; // 当前 Z 轴角速度

    imu_elapsed_ms += H2024_LOOP_MS; // 累计 MPU6050 采样间隔
    if (imu_elapsed_ms < H2024_IMU_PERIOD_MS) return; // 未到 10ms 不访问 I2C
    imu_elapsed_ms = 0U; // 开始新一轮积分周期

    if (mpu6050_read_gyro_z(&gyro_z_dps) == 0U) {
        telemetry.imu_ready = 0U; // I2C 读取异常，禁止继续执行无轨直线
        return;
    }

    yaw_deg += (gyro_z_dps - gyro_bias_dps) * H2024_GYRO_SIGN
        * ((float) H2024_IMU_PERIOD_MS / 1000.0f); // 角速度积分得到相对航向
    yaw_deg = h2024_wrap_angle(yaw_deg); // 防止角度持续累积溢出
}

static uint8_t h2024_calibrate_imu(void)
{
    float sum = 0.0f; // Z 轴静止角速度累加值
    float gyro_z_dps; // 单次读取到的角速度
    uint16_t valid_count = 0U; // 成功读取的样本数量

    if (mpu6050_init() == 0U) return 0U; // 芯片不存在或 I2C 异常

    for (uint16_t i = 0U; i < H2024_GYRO_CALIBRATION_SAMPLES; i++) {
        if (mpu6050_read_gyro_z(&gyro_z_dps) != 0U) {
            sum += gyro_z_dps; // 只累计读取成功的数据
            valid_count++;
        }
        vTaskDelay(pdMS_TO_TICKS(H2024_IMU_PERIOD_MS)); // 校准期间小车必须静止
    }

    if (valid_count < (H2024_GYRO_CALIBRATION_SAMPLES * 3U / 4U)) {
        return 0U; // 成功率过低，认为 I2C 或接线不可靠
    }

    gyro_bias_dps = sum / (float) valid_count; // 计算静止零偏
    yaw_deg = 0.0f; // 校准完成后当前位置定义为 0 度
    target_yaw_deg = 0.0f;
    imu_elapsed_ms = 0U;

    return 1U;
}

static void h2024_select_segments(void)
{
    if (telemetry.selected_task == 1U) {
        segments = task1_segments; // 要求 1 只跑 A -> B
        segment_count = (uint8_t) (sizeof(task1_segments) / sizeof(task1_segments[0]));
        target_laps = 1U;
    } else if (telemetry.selected_task == 2U) {
        segments = task2_segments; // 要求 2 跑水平直线和两个半圆
        segment_count = (uint8_t) (sizeof(task2_segments) / sizeof(task2_segments[0]));
        target_laps = 1U;
    } else {
        segments = task3_segments; // 要求 3、4 使用同一条斜线路线
        segment_count = (uint8_t) (sizeof(task3_segments) / sizeof(task3_segments[0]));
        target_laps = (telemetry.selected_task == 4U) ? 4U : 1U; // 要求 4 自动重复四圈
    }
}

static void h2024_enter_segment(void)
{
    control_reset_distance(); // 新路段从 0cm 开始测距
    last_track_error = 0; // 新弧线不沿用上一段的循迹 D 项
    arc_elapsed_ms = 0U; // 新弧线重新计算入口找线宽限
    arc_line_acquired = 0U; // 每条弧线都必须重新看到黑线，不能直接沿用旧 raw 偏差

    if (segments[segment_index].type == H2024_SEGMENT_STRAIGHT) {
        telemetry.state = H2024_STATE_STRAIGHT; // 进入 MPU6050 航向保持直线
        target_yaw_deg = segments[segment_index].heading_deg; // 使用题面几何关系给出目标航向
    } else {
        telemetry.state = H2024_STATE_ARC; // 进入八路灰度循迹半圆弧
    }
}

static void h2024_start_mission(void)
{
    h2024_select_segments(); // 根据按键选择加载对应路线
    segment_index = 0U; // 从路线第一段开始
    current_lap = 1U; // 从第一圈开始
    mission_running = 1U; // 标记任务开始运行
    yaw_deg = 0.0f; // A 点摆正后启动，当前朝向定义为 0 度
    target_yaw_deg = 0.0f;
    h2024_enter_segment(); // 开始执行第一段
}

static void h2024_finish_mission(void)
{
    control_stop(); // 到达终点后停车
    mission_running = 0U; // 当前路线已经结束
    telemetry.state = H2024_STATE_FINISHED; // 串口显示完成状态
    h2024_alert(H2024_FINISH_ALERT_MS); // 终点给较长的声光提示
}

static void h2024_fail_mission(void)
{
    control_stop(); // 传感器异常时立即停车
    mission_running = 0U; // 退出当前路线
    telemetry.state = H2024_STATE_ERROR; // 串口显示错误状态
    h2024_alert(H2024_ERROR_ALERT_MS); // 给出较长错误提示
}

static void h2024_complete_segment(void)
{
    h2024_alert(H2024_POINT_ALERT_MS); // 每经过 B、C、D、A 点声光提示一次
    segment_index++; // 准备进入下一路段

    if (segment_index < segment_count) {
        h2024_enter_segment(); // 本圈还有后续路段
        return;
    }

    if (current_lap < target_laps) {
        current_lap++; // 要求 4 自动进入下一圈
        segment_index = 0U; // 下一圈重新从 A -> C 开始
        h2024_enter_segment();
        return;
    }

    h2024_finish_mission(); // 所有路段和圈数都完成
}

static void h2024_drive_straight(void)
{
    float yaw_error = h2024_wrap_angle(target_yaw_deg - yaw_deg); // 当前航向偏差
    float turn = h2024_limit(yaw_error * H2024_STRAIGHT_YAW_KP,
        H2024_STRAIGHT_MAX_TURN_RPM); // 航向误差换算为差速修正

    control_set_target(H2024_STRAIGHT_BASE_RPM + turn,
        H2024_STRAIGHT_BASE_RPM - turn); // 两轮始终保持正转，满足题目要求
}

static void h2024_drive_arc(const TrackInfo *track)
{
    float turn = h2024_limit(
        (float) track->error * H2024_ARC_KP +
        (float) (track->error - last_track_error) * H2024_ARC_KD,
        H2024_ARC_MAX_TURN_RPM); // 八路灰度偏差通过 PD 换算成弧线差速

    last_track_error = track->error; // 保存本次偏差，供下一次 D 项使用
    control_set_target(H2024_ARC_BASE_RPM + turn,
        H2024_ARC_BASE_RPM - turn); // 修正量小于基础速度，两轮不会反转
}

static void h2024_update_motion(const TrackInfo *track)
{
    float distance_cm = control_get_distance_cm(); // 获取当前路段累计距离

    if (telemetry.state == H2024_STATE_STRAIGHT) {
        if (telemetry.imu_ready == 0U) {
            h2024_fail_mission(); // 无轨直线必须依赖 MPU6050
            return;
        }
        h2024_drive_straight(); // MPU6050 航向闭环
    } else if (telemetry.state == H2024_STATE_ARC) {
        if (arc_elapsed_ms < UINT16_MAX - H2024_LOOP_MS) {
            arc_elapsed_ms += H2024_LOOP_MS; // 记录进入弧线后的时间，避免无符号溢出
        }

        if (track_age_ms >= H2024_TRACK_TIMEOUT_MS) {
            h2024_fail_mission(); // 半圆弧阶段 raw 通讯超时
            return;
        }

        if (track->raw != 0U) {
            arc_line_acquired = 1U; // 至少一个探头压到黑线，确认已经进入当前弧线
        }

        if (arc_line_acquired == 0U) {
            if (arc_elapsed_ms < H2024_ARC_ACQUIRE_MS) {
                control_set_target(H2024_ARC_ACQUIRE_RPM,
                    H2024_ARC_ACQUIRE_RPM); // 探头尚未压到黑线时低速向前搜索
                return;
            }

            h2024_fail_mission(); // 超过入口宽限仍未找到线，停车等待检查
            return;
        }

        if (track->valid == 0U) {
            h2024_fail_mission(); // 已经压到线却无法计算偏差，停车等待检查
            return;
        }

        h2024_drive_arc(track); // 八路灰度循迹
    }

    if (distance_cm >= segments[segment_index].distance_cm) {
        h2024_complete_segment(); // 当前路段里程达到默认值，进入下一个点
    }
}

void h2024_task_loop(void)
{
    TickType_t last_wake_time = xTaskGetTickCount(); // 记录任务固定周期起点

    telemetry.selected_task = 1U; // 上电默认选中要求 1
    telemetry.state = H2024_STATE_WAIT; // 上电先待机，等待按键
    telemetry.imu_ready = h2024_calibrate_imu(); // 上电静止约 2 秒校准 MPU6050
    h2024_set_signal(0U); // 校准结束后确保声光提示关闭

    while (1) {
        uint8_t mode_pressed = h2024_button_pressed(
            &mode_button, H2024_BUTTONS_MODE_PIN); // 读取 MODE 按键
        uint8_t start_pressed = h2024_button_pressed(
            &start_button, H2024_BUTTONS_START_PIN); // 读取 START 按键
        TrackInfo track; // 保存最新八路灰度状态

        if (Track_receive() != 0U) {
            track_age_ms = 0U; // 收到一帧新的 0xFE/raw/0xEF
        } else if (track_age_ms < H2024_TRACK_TIMEOUT_MS) {
            track_age_ms += H2024_LOOP_MS; // 没有新帧时累计等待时间
        }
        track = Track_get_info(); // 获取最新 raw 和循迹偏差

        h2024_update_imu(); // 每 10ms 更新一次航向角
        h2024_update_signal(); // 非阻塞更新蜂鸣器和 LED

        if (mission_running != 0U) {
            if (start_pressed != 0U) {
                control_stop(); // 运行中再次按 START 作为急停
                mission_running = 0U;
                telemetry.state = H2024_STATE_WAIT;
            } else {
                h2024_update_motion(&track); // 执行当前直线或弧线路段
            }
        } else if ((telemetry.state == H2024_STATE_WAIT) ||
                   (telemetry.state == H2024_STATE_FINISHED) ||
                   (telemetry.state == H2024_STATE_ERROR)) {
            control_stop(); // 待机、完成或异常状态始终保持停车

            if (mode_pressed != 0U) {
                telemetry.selected_task++; // MODE 每按一次切换到下一项要求
                if (telemetry.selected_task > 4U) telemetry.selected_task = 1U;
                telemetry.state = H2024_STATE_WAIT;
                h2024_alert(H2024_POINT_ALERT_MS); // 切换模式时短鸣一次
            }
            if (start_pressed != 0U) {
                if (telemetry.imu_ready != 0U) {
                    h2024_start_mission(); // MPU6050 正常时启动当前要求
                } else {
                    telemetry.state = H2024_STATE_ERROR; // MPU6050 未就绪，拒绝启动
                    h2024_alert(H2024_ERROR_ALERT_MS);
                }
            }
        }

        telemetry.segment = segment_index; // 更新串口显示的当前路段
        telemetry.lap = current_lap; // 更新串口显示的当前圈数
        telemetry.track_raw = track.raw; // 更新串口显示的八路 raw
        telemetry.track_error = track.error; // 更新串口显示的循迹偏差
        telemetry.track_valid = track.valid; // 更新串口显示的循迹有效状态
        telemetry.yaw_deg = yaw_deg; // 更新串口显示的当前航向角
        telemetry.target_yaw_deg = target_yaw_deg; // 更新串口显示的目标航向角
        telemetry.distance_cm = control_get_distance_cm(); // 更新串口显示的本路段里程

        vTaskDelayUntil(&last_wake_time,
            pdMS_TO_TICKS(H2024_LOOP_MS)); // 每 5ms 唤醒一次
    }
}

void h2024_get_telemetry(H2024Telemetry *data)
{
    uint32_t primask; // 保存进入函数前的中断状态

    if (data == NULL) return; // 空指针保护

    primask = __get_PRIMASK(); // 保存原中断状态
    __disable_irq(); // 复制快照时避免任务更新一半数据
    *data = telemetry; // 一次复制完整状态快照
    __set_PRIMASK(primask); // 恢复原中断状态
}
