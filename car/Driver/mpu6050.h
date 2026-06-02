#ifndef __MPU6050_H_
#define __MPU6050_H_ // 防止 mpu6050.h 被重复包含

#include <stdint.h>

#define MPU6050_ADDRESS (0x68U) // AD0 接 GND 时的 MPU6050 I2C 地址

uint8_t mpu6050_init(void); // 初始化 MPU6050，成功返回 1
uint8_t mpu6050_read_gyro_z(float *gyro_z_dps); // 读取 Z 轴角速度，单位 deg/s

#endif
