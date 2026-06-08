#ifndef __ODOM_H_
#define __ODOM_H_

#include "mpu6050/mpu_port.h"
#include "Driver/encode.h"
#include "odom/odom.h"

#define WHEEL_CIR       (0.132f)       // 轮子周长

typedef struct{
    float theta;                       // 里程计朝向角
    float distance;                    // 里程计本阶段累计路程
    float delta_distance;              // 里程计本周期位移
}Odom;

void odom_init();           // 初始化
void odom_reset(Odom *odom);
void odom_update(Odom *odom);

#endif