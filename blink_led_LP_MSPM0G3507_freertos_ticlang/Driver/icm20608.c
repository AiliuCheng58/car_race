#include "icm20608.h"
#include "source/delay.h"
#include "source/uart.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/driverlib/dl_uart.h"
#include "ti_msp_dl_config.h"

ICM_Data g_imu_raw;
volatile uint8_t g_imu_ready = 0;

uint8_t icm_init(void){
    uint8_t res;
    delay_MS(100);
    if(!i2c_ReadByte(ICM_ADDR, WHO_AM_I, &res))
        return 123;
    DL_UART_transmitDataBlocking(UART_0_INST, res);
    DL_UART_transmitDataBlocking(UART_0_INST, 'F');
    if(res != WHO_AM_I_VALUE)
        return 231;
    DL_UART_transmitDataBlocking(UART_0_INST, 'G');
    if(!i2c_SendByte(ICM_ADDR, PWR_MGMT_1, DEVICE_RESET))
        return 2;
    DL_UART_transmitDataBlocking(UART_0_INST, 'H');
    delay_MS(100);

    if(!i2c_SendByte(ICM_ADDR, PWR_MGMT_1, AWAKE))
        return 3;
    DL_UART_transmitDataBlocking(UART_0_INST, 'I');
    if(!i2c_SendByte(ICM_ADDR, PWR_MGMT_2, START))
        return 4;
    DL_UART_transmitDataBlocking(UART_0_INST, 'J');
    if(!icm_set_lpf(50))
        return 5;
    DL_UART_transmitDataBlocking(UART_0_INST, 'K');
    if(!icm_set_accel_lpf(50))
        return 6;
    DL_UART_transmitDataBlocking(UART_0_INST, 'L');
    if(!icm_set_gyro_fsr(GYRO_FS_250DPS))
        return 7;
    DL_UART_transmitDataBlocking(UART_0_INST, 'M');
    if(!icm_set_accel_fsr(ACCEL_FS_2G))
        return 8;
    DL_UART_transmitDataBlocking(UART_0_INST, 'N');
    if(!icm_set_rate(100))
        return 9;

    delay_MS(10);

    return 10;
}

uint8_t icm_ReadRaw(ICM_Data *data){
    uint8_t num[RAW_DATA_LEN];
    if(!i2c_ReadBytes(ICM_ADDR, ACCEL_XOUT_H, num, RAW_DATA_LEN))
        return 0;

    data->ax = (int16_t)((uint16_t)num[0] << 8 | num[1]);
    data->ay = (int16_t)((uint16_t)num[2] << 8 | num[3]);
    data->az = (int16_t)((uint16_t)num[4] << 8 | num[5]);

    data->temp = (int16_t)((uint16_t)num[6] << 8 | num[7]);

    data->gx = (int16_t)((uint16_t)num[8] << 8 | num[9]);
    data->gy = (int16_t)((uint16_t)num[10] << 8 | num[11]);
    data->gz = (int16_t)((uint16_t)num[12] << 8 | num[13]);
    
    return 1;
}

uint8_t icm_set_rate(uint16_t rate){
    uint8_t data;

    if(rate > 1000)
        rate = 1000;
    else if(rate < 4)
        rate = 4;

    data = (uint8_t)(1000 / rate - 1);

    return i2c_SendByte(ICM_ADDR, SMPLRT_DIV, data);
}

uint8_t icm_set_lpf(uint16_t lpf){
    uint8_t data;
    uint8_t reg;

    if(lpf >= 188)      data = 1;
    else if(lpf >= 98)  data = 2;
    else if(lpf >= 42)  data = 3;
    else if(lpf >= 20)  data = 4;
    else if(lpf >= 10)  data = 5;
    else                data = 6;

    if(!i2c_ReadByte(ICM_ADDR, CONFIG, &reg))
        return 0;

    reg &= ~0x07;
    reg |= data;

    return i2c_SendByte(ICM_ADDR, CONFIG, reg);
}

uint8_t icm_set_accel_lpf(uint16_t lpf){
    uint8_t data;
    uint8_t reg;

    if(lpf >= 218)      data = 1;
    else if(lpf >= 99)  data = 2;
    else if(lpf >= 45)  data = 3;
    else if(lpf >= 21)  data = 4;
    else if(lpf >= 10)  data = 5;
    else                data = 5;

    if(!i2c_ReadByte(ICM_ADDR, ACCEL_CONFIG2, &reg))
        return 0;

    reg &= ~0x07;
    reg |= data;

    return i2c_SendByte(ICM_ADDR, ACCEL_CONFIG2, reg);
}

uint8_t icm_set_gyro_fsr(uint8_t fsr){
    uint8_t reg;

    if(!i2c_ReadByte(ICM_ADDR, GYRO_CONFIG, &reg))
        return 0;

    reg &= ~(0x07 << 3);
    reg |= (fsr << 3);

    return i2c_SendByte(ICM_ADDR, GYRO_CONFIG, reg);
}

uint8_t icm_set_accel_fsr(uint8_t fsr){
    uint8_t reg;

    if(!i2c_ReadByte(ICM_ADDR, ACCEL_CONFIG, &reg))
        return 0;

    reg &= ~(0x07 << 3);
    reg |= (fsr << 3);

    return i2c_SendByte(ICM_ADDR, ACCEL_CONFIG, reg);
}
