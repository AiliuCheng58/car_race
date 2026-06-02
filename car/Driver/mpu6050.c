#include "mpu6050.h"

#include "delay.h"
#include "i2c.h"

#define MPU6050_REG_SMPLRT_DIV   (0x19U) // 陀螺仪采样分频寄存器
#define MPU6050_REG_CONFIG       (0x1AU) // 数字低通滤波配置寄存器
#define MPU6050_REG_GYRO_CONFIG  (0x1BU) // 陀螺仪量程配置寄存器
#define MPU6050_REG_GYRO_ZOUT_H  (0x47U) // Z 轴角速度高字节寄存器
#define MPU6050_REG_PWR_MGMT_1   (0x6BU) // 电源管理寄存器
#define MPU6050_REG_WHO_AM_I     (0x75U) // 芯片身份寄存器

#define MPU6050_DEVICE_RESET     (0x80U) // 写入 PWR_MGMT_1 后复位芯片
#define MPU6050_CLOCK_PLL_XGYRO  (0x01U) // 唤醒芯片并使用 X 轴陀螺仪 PLL
#define MPU6050_WHO_AM_I_VALUE   (0x68U) // MPU6050 正常返回的身份值
#define MPU6050_GYRO_SCALE_250   (131.0f) // +/-250 deg/s 量程下每 deg/s 对应的原始值

uint8_t mpu6050_init(void)
{
    uint8_t who_am_i = 0U; // 保存芯片身份寄存器

    delay_ms(100U); // 上电后给 MPU6050 留出启动时间

    if (i2c_ReadByte(MPU6050_ADDRESS, MPU6050_REG_WHO_AM_I, &who_am_i) == 0U) {
        return 0U; // I2C 无法读到芯片
    }
    if (who_am_i != MPU6050_WHO_AM_I_VALUE) {
        return 0U; // 地址上存在设备，但不是 MPU6050
    }

    if (i2c_SendByte(MPU6050_ADDRESS, MPU6050_REG_PWR_MGMT_1,
            MPU6050_DEVICE_RESET) == 0U) {
        return 0U; // 复位命令发送失败
    }
    delay_ms(100U); // 等待芯片复位完成

    if (i2c_SendByte(MPU6050_ADDRESS, MPU6050_REG_PWR_MGMT_1,
            MPU6050_CLOCK_PLL_XGYRO) == 0U) {
        return 0U; // 唤醒并选择 PLL 时钟
    }
    if (i2c_SendByte(MPU6050_ADDRESS, MPU6050_REG_SMPLRT_DIV, 9U) == 0U) {
        return 0U; // 1kHz / (1 + 9) = 100Hz
    }
    if (i2c_SendByte(MPU6050_ADDRESS, MPU6050_REG_CONFIG, 3U) == 0U) {
        return 0U; // 约 44Hz 低通滤波，减少车体振动对航向的影响
    }
    if (i2c_SendByte(MPU6050_ADDRESS, MPU6050_REG_GYRO_CONFIG, 0U) == 0U) {
        return 0U; // Z 轴使用 +/-250 deg/s 量程
    }

    return 1U; // 初始化完成
}

uint8_t mpu6050_read_gyro_z(float *gyro_z_dps)
{
    uint8_t data[2]; // 保存 Z 轴角速度的高低字节
    int16_t raw; // 合并后的带符号原始值

    if (gyro_z_dps == 0) {
        return 0U; // 空指针保护
    }
    if (i2c_ReadBytes(MPU6050_ADDRESS, MPU6050_REG_GYRO_ZOUT_H,
            data, sizeof(data)) == 0U) {
        return 0U; // I2C 读取失败
    }

    raw = (int16_t) (((uint16_t) data[0] << 8) | data[1]); // 合并高低字节
    *gyro_z_dps = (float) raw / MPU6050_GYRO_SCALE_250; // 换算成 deg/s

    return 1U;
}
