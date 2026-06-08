#ifndef __ODOM_H_
#define __ODOM_H_

#include "mpu6050/mpu_port.h"
#include "Driver/encode.h"

typedef struct{
    float theta_init;                  // 初始朝向
    float target_A2C;                  // A到C的角度
    float target_B2D;                  // B到D的角度

    float theta;                       // 里程计朝向角
    float distance;                    // 里程计本阶段累计路程
    float delta_distance;              // 里程计本周期位移

    int32_t encode_last_left;            // 编码器上次左轮计数
    int32_t encode_last_right;           // 编码器上次右轮计数

    float death;                       // 本阶段累计路程+-死区如果在赛道长度附近的话就切阶段
}Odom;

extern volatile Odom odom;

void odom_init(void);           // 初始化
void odom_clear(void);
void odom_update(void);
float odom_wrap180(float angle);

#endif