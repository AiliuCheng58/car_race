#ifndef __ICM20608_H_
#define __ICM20608_H_

#include "head.h"

/* I2C 地址 */
#define ICM_ADDR     0x68

/* 设备 ID */
#define WHO_AM_I          0x75
//#define WHO_AM_I_VALUE    0xAF
#define WHO_AM_I_VALUE    0xAE

/* 电源管理 */
#define PWR_MGMT_1        0x6B
#define PWR_MGMT_2        0x6C

/* 采样率 / 滤波 / 量程 */
#define SMPLRT_DIV        0x19
#define CONFIG            0x1A
#define GYRO_CONFIG       0x1B
#define ACCEL_CONFIG      0x1C
#define ACCEL_CONFIG2     0x1D

/* 六轴数据起始地址 */
#define ACCEL_XOUT_H      0x3B
#define RAW_DATA_START    ACCEL_XOUT_H
#define RAW_DATA_LEN      14

/* 初始化 */
#define DEVICE_RESET      0x80
#define AWAKE             0x00
#define START             0x00

/* 量程：±250dps，±2g */
#define GYRO_FS_250DPS    0x00
#define ACCEL_FS_2G       0x00

typedef struct
{
    int16_t ax;
    int16_t ay;
    int16_t az;

    int16_t gx;
    int16_t gy;
    int16_t gz;

    int16_t temp;
    float Temp;

}ICM_Data;

extern ICM_Data g_imu_raw;
extern volatile uint8_t g_imu_ready;
extern volatile uint8_t g_imu_init_status;
extern volatile uint8_t g_imu_read_status;
extern volatile uint32_t g_imu_sample_count;
uint8_t icm_init(void);
uint8_t icm_ReadRaw(ICM_Data *data);
uint8_t icm_set_rate(uint16_t rate);
uint8_t icm_set_lpf(uint16_t lpf);
uint8_t icm_set_accel_lpf(uint16_t lpf);
uint8_t icm_set_gyro_fsr(uint8_t fsr);
uint8_t icm_set_accel_fsr(uint8_t fsr);

#endif
