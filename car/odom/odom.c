#include "odom.h"
#include "encode.h"
#include "mpu6050/mpu_port.h"

#define DELTA   (102.7f)        //B2D与A2C的偏离角度

volatile float YAW_A2C;
volatile float YAW_B2D;

Odom *odom;

void odom_init(){
    float pitch, roll, yaw;
    DMP_Read_Data(&pitch, &roll, &yaw);
    odom_reset(odom);
    odom->theta = yaw;
    YAW_A2C = yaw;
    YAW_B2D = yaw + DELTA;
}

void odom_reset(Odom *odom){
    odom->delta_distance = 0;
    odom->distance = 0;
    odom->theta = 0;
}

void odom_update(Odom *odom){
    float s_left, s_right, ds;
    float pitch, roll, yaw;
    EncoderSample tmp = encoder_get_total();
    DMP_Read_Data(&pitch, &roll, &yaw);

    s_left = tmp.left * WHEEL_CIR / ENCODER_LEFT_COUNTS_PER_REV;
    s_right = tmp.right * WHEEL_CIR / ENCODER_RIGHT_COUNTS_PER_REV;

    ds = (s_right + s_left) / 2;

    odom->theta = yaw;
    odom->delta_distance = ds;
    odom->distance += ds;

}