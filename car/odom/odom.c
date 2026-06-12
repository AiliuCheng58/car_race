#include "odom.h"
#include "delay/delay.h"
#include "encode/encode.h"
#include "mpu6050/mpu_port.h"

#define WHEEL_CIR       (0.132f)       // 轮子周长
#define DEATH           (0.08f)        // 手动设置死区
#define DELTA_A2C       (-38.66f)       // A2C的偏离角度
#define DELTA_B2D       (141.34F)      // B2D的偏离角度
#define DMP_INIT_RETRY  (20U)          // 等待 DMP 首帧的最大次数
#define DMP_WAIT_MS     (10U)          // DMP 100Hz，约 10ms 一帧

volatile Odom  odom;

volatile uint8_t flag = 1;

static uint8_t Odom_read_yaw(float *yaw)
{
    float pitch, roll, tmp_yaw;

    if (yaw == NULL) {
        return 0U;
    }

    if (DMP_Read_Data(&pitch, &roll, &tmp_yaw) != 0) {
        return 0U;
    }

    *yaw = tmp_yaw;
    return 1U;
}

void odom_init(void)
{
    float yaw = 0.0f;
    EncoderSample tmp = encoder_get_total();

    for (uint8_t i = 0U; i < DMP_INIT_RETRY; i++) {
        if (Odom_read_yaw(&yaw) != 0U) {
            break;
        }
        delay_ms(DMP_WAIT_MS);
    }

    odom_clear();

    odom.delta_distance = 0;
    odom.encode_last_left = tmp.left;
    odom.encode_last_right = tmp.right;
    odom.theta = yaw;
    odom.theta_init = yaw;
    odom.death = DEATH;
    odom.target_A2C = odom_wrap180(yaw + DELTA_A2C);
    odom.target_B2D = odom_wrap180(yaw + DELTA_B2D);
}

void odom_clear(void)
{
    odom.distance = 0;
    odom.delta_distance = 0;
}

void odom_recalibrate_targets(void)
{
    float yaw;

    if (Odom_read_yaw(&yaw) == 0U) {
        return;
    }

    odom.theta = yaw;
    odom.target_A2C = odom_wrap180(yaw + DELTA_A2C);
    odom.target_B2D = odom_wrap180(yaw + DELTA_B2D);
}

void odom_update(void)
{
    float s_left, s_right, ds;
    float yaw;
    EncoderSample tmp = encoder_get_total();

    s_left = (tmp.left - odom.encode_last_left) * WHEEL_CIR / ENCODER_LEFT_COUNTS_PER_REV;
    s_right = (tmp.right - odom.encode_last_right) * WHEEL_CIR / ENCODER_RIGHT_COUNTS_PER_REV;

    ds = (s_right + s_left) / 2;
    if (flag)
        if (Odom_read_yaw(&yaw) != 0U)
            odom.theta = yaw;
    
    odom.delta_distance = ds;
    odom.distance += ds;

    odom.encode_last_left = tmp.left;
    odom.encode_last_right = tmp.right;

    flag = 1 - flag;
}

float odom_wrap180(float angle)
{
    while (angle > 180.0f) {
        angle -= 360.0f;
    }

    while (angle < -180.0f) {
        angle += 360.0f;
    }

    return angle;
}
