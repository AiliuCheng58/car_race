#include "mpu_port.h"
#include "delay/delay.h"
#include "inv_mpu.h"
#include "inv_mpu_dmp_motion_driver.h"

#include <math.h>
#include <stddef.h>

#define MPU_I2C_TIMEOUT_COUNT          (100000UL)
#define MPU_START_DELAY_MS             (100UL)
#define MPU_SAMPLE_RATE_HZ             (100U)
#define MPU_DMP_FEATURES               (DMP_FEATURE_6X_LP_QUAT | DMP_FEATURE_GYRO_CAL)
#define MPU_Q30                        (1073741824.0f)
#define MPU_RAD_TO_DEG                 (57.2957795f)

static volatile unsigned long mpu_ms = 0UL;
static const signed char gyro_orientation[9] = {
    1, 0, 0,
    0, 1, 0,
    0, 0, 1
};

void mget_ms(unsigned long *time)
{
    if (time != NULL) {
        *time = mpu_ms;
    }
}

void MPU_delay_ms(unsigned long ms)
{
    delay_ms((uint32_t) ms);
    mpu_ms += ms;
}

int MPU_Write_Len(unsigned char addr, unsigned char reg, unsigned char len, unsigned char *buf)
{
    volatile uint32_t timeout = MPU_I2C_TIMEOUT_COUNT;
    while (!(DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        if (--timeout == 0) return -1;
    }
    
    DL_I2C_transmitControllerData(MPU6050_INST, reg);
    DL_I2C_startControllerTransfer(MPU6050_INST, addr, DL_I2C_CONTROLLER_DIRECTION_TX, len + 1);
    
    for (uint16_t i = 0U; i < len; i++) {
        timeout = MPU_I2C_TIMEOUT_COUNT;
        while (DL_I2C_isControllerTXFIFOFull(MPU6050_INST)) {
            if (--timeout == 0) return -2;
        }
        DL_I2C_transmitControllerData(MPU6050_INST, buf[i]);
    }
    
    timeout = MPU_I2C_TIMEOUT_COUNT;
    while (DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_BUSY) {
        if (--timeout == 0) return -3;
    }
    timeout = MPU_I2C_TIMEOUT_COUNT;
    while (!(DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        if (--timeout == 0) return -4;
    }
    return 0;
}

int MPU_Read_Len(unsigned char addr, unsigned char reg, unsigned char len, unsigned char *buf)
{
    volatile uint32_t timeout; 
    
    timeout = MPU_I2C_TIMEOUT_COUNT;
    while (!(DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        if (--timeout == 0) return -1;
    }
    
    DL_I2C_transmitControllerData(MPU6050_INST, reg);
    DL_I2C_startControllerTransfer(MPU6050_INST, addr, DL_I2C_CONTROLLER_DIRECTION_TX, 1);
    
    timeout = MPU_I2C_TIMEOUT_COUNT;
    while (DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_BUSY) {
        if (--timeout == 0) return -2;
    }
    timeout = MPU_I2C_TIMEOUT_COUNT;
    while (!(DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        if (--timeout == 0) return -3;
    }

    DL_I2C_startControllerTransfer(MPU6050_INST, addr, DL_I2C_CONTROLLER_DIRECTION_RX, len);
    
    for (uint16_t i = 0U; i < len; i++) {
        timeout = MPU_I2C_TIMEOUT_COUNT;
        while (DL_I2C_isControllerRXFIFOEmpty(MPU6050_INST)) {
            
            if (DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) return -4;
            
            if (--timeout == 0) return -5; 
        }
        buf[i] = DL_I2C_receiveControllerData(MPU6050_INST);
    }
    
    timeout = MPU_I2C_TIMEOUT_COUNT;
    while (DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_BUSY) {
        if (--timeout == 0) return -6;
    }
    timeout = MPU_I2C_TIMEOUT_COUNT;
    while (!(DL_I2C_getControllerStatus(MPU6050_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        if (--timeout == 0) return -7;
    }
    return 0;
}

unsigned short inv_row_2_scale(const signed char *row)
{
    unsigned short b;
    if (row[0] > 0) b = 0;
    else if (row[0] < 0) b = 4;
    else if (row[1] > 0) b = 1;
    else if (row[1] < 0) b = 5;
    else if (row[2] > 0) b = 2;
    else if (row[2] < 0) b = 6;
    else b = 7;
    return b;
}

unsigned short inv_orientation_matrix_to_scalar(const signed char *mtx)
{
    unsigned short scalar;
    scalar = inv_row_2_scale(mtx);
    scalar |= inv_row_2_scale(mtx + 3) << 3;
    scalar |= inv_row_2_scale(mtx + 6) << 6;
    return scalar;
}

int DMP_Init(void)
{
    int res;

    MPU_delay_ms(MPU_START_DELAY_MS);
    res = mpu_init();
    if (res != 0) return res;

    res = mpu_set_sensors(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    if (res != 0) return res;

    res = mpu_configure_fifo(INV_XYZ_GYRO | INV_XYZ_ACCEL);
    if (res != 0) return res;

    res = mpu_set_sample_rate(MPU_SAMPLE_RATE_HZ);
    if (res != 0) return res;

    res = dmp_load_motion_driver_firmware();
    if (res != 0) return res;

    res = dmp_set_orientation(inv_orientation_matrix_to_scalar(gyro_orientation));
    if (res != 0) return res;

    res = dmp_enable_feature(MPU_DMP_FEATURES);
    if (res != 0) return res;

    res = dmp_set_fifo_rate(MPU_SAMPLE_RATE_HZ);
    if (res != 0) return res;

    return mpu_set_dmp_state(1);
}

int DMP_Read_Data(float *pitch, float *roll, float *yaw)
{
    short gyro[3], accel[3], sensors;
    unsigned char more;
    long quat[4];

    if ((pitch == NULL) || (roll == NULL) || (yaw == NULL)) {
        return -1;
    }

    if (dmp_read_fifo(gyro, accel, quat, NULL, &sensors, &more) == 0) {
        if (sensors & INV_WXYZ_QUAT) {
            float q0 = quat[0] / MPU_Q30;
            float q1 = quat[1] / MPU_Q30;
            float q2 = quat[2] / MPU_Q30;
            float q3 = quat[3] / MPU_Q30;

            *pitch = asin(-2.0f * q1 * q3 + 2.0f * q0 * q2) * MPU_RAD_TO_DEG;
            *roll = atan2(2.0f * q2 * q3 + 2.0f * q0 * q1,
                -2.0f * q1 * q1 - 2.0f * q2 * q2 + 1.0f) * MPU_RAD_TO_DEG;
            *yaw = atan2(2.0f * (q1 * q2 + q0 * q3),
                q0 * q0 + q1 * q1 - q2 * q2 - q3 * q3) * MPU_RAD_TO_DEG;
            return 0;
        }
    }

    return -2;
}

