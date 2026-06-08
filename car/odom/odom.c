#include "odom.h"
#include "encode.h"
#include "mpu6050/mpu_port.h"

#define WHEEL_CIR       (0.132f)       // 轮子周长
#define DEATH           (0.08f)        // 手动设置死区
#define DELTA_A2C       (38.66f)       // A2C的偏离角度
#define DELTA_B2D       (141.34F)      // B2D的偏离角度

volatile Odom  odom;

void odom_init(){
    float pitch, roll, yaw;
    EncoderSample tmp = encoder_get_total();

    DMP_Read_Data(&pitch, &roll, &yaw);
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

void odom_clear(){
    odom.distance = 0;
    odom.delta_distance = 0;
}

void odom_update(){
    float s_left, s_right, ds;
    float pitch, roll, yaw;
    EncoderSample tmp = encoder_get_total();
    DMP_Read_Data(&pitch, &roll, &yaw);

    s_left = (tmp.left - odom.encode_last_left) * WHEEL_CIR / ENCODER_LEFT_COUNTS_PER_REV;
    s_right = (tmp.right - odom.encode_last_right) * WHEEL_CIR / ENCODER_RIGHT_COUNTS_PER_REV;

    ds = (s_right + s_left) / 2;

    odom.theta = yaw;
    odom.delta_distance = ds;
    odom.distance += ds;

    odom.encode_last_left = tmp.left;
    odom.encode_last_right = tmp.right;
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