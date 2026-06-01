#include "icm20608.h"
#include "source/delay.h"
#include "source/uart.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/driverlib/dl_uart.h"
#include "ti_msp_dl_config.h"

ICM_Data g_imu_raw;
volatile uint8_t g_imu_ready = 0;
volatile uint8_t g_imu_init_status = 0;
volatile uint8_t g_imu_read_status = 0;
volatile uint32_t g_imu_sample_count = 0;

static void icm_delay_ms(uint32_t ms)
{
    if (xTaskGetSchedulerState() == taskSCHEDULER_NOT_STARTED) {
        delay_MS(ms);
    } else {
        delay_ms(ms);
    }
}

uint8_t icm_init(void){
    uint8_t res;
    uint8_t ok = 0;

    icm_delay_ms(200);
    for (uint8_t i = 0; i < 10U; i++) {
        if(i2c_ReadByte(ICM_ADDR, WHO_AM_I, &res)){
            ok = 1;
            break;
        }
        g_imu_init_status = 123;
        icm_delay_ms(20);
    }
    if(!ok){
        return 123;
    }
    if(res != WHO_AM_I_VALUE){
        g_imu_init_status = 231;
        return 231;
    }
    if(!i2c_SendByte(ICM_ADDR, PWR_MGMT_1, DEVICE_RESET)){
        g_imu_init_status = 2;
        return 2;
    }
    icm_delay_ms(100);

    if(!i2c_SendByte(ICM_ADDR, PWR_MGMT_1, AWAKE)){
        g_imu_init_status = 3;
        return 3;
    }
    if(!i2c_SendByte(ICM_ADDR, PWR_MGMT_2, START)){
        g_imu_init_status = 4;
        return 4;
    }
    if(!icm_set_lpf(50)){
        g_imu_init_status = 5;
        return 5;
    }
    if(!icm_set_accel_lpf(50)){
        g_imu_init_status = 6;
        return 6;
    }
    if(!icm_set_gyro_fsr(GYRO_FS_250DPS)){
        g_imu_init_status = 7;
        return 7;
    }
    if(!icm_set_accel_fsr(ACCEL_FS_2G)){
        g_imu_init_status = 8;
        return 8;
    }
    if(!icm_set_rate(100)){
        g_imu_init_status = 9;
        return 9;
    }

    icm_delay_ms(10);

    g_imu_init_status = 10;
    return 10;
}

uint8_t icm_ReadRaw(ICM_Data *data){
    uint8_t num[RAW_DATA_LEN];

    for (uint8_t i = 0; i < RAW_DATA_LEN; i++) {
        if (!i2c_ReadByte(ICM_ADDR, (uint8_t)(ACCEL_XOUT_H + i), &num[i])) {
            return 0;
        }
    }

    data->ax = (int16_t)((uint16_t)num[0] << 8 | num[1]);
    data->ay = (int16_t)((uint16_t)num[2] << 8 | num[3]);
    data->az = (int16_t)((uint16_t)num[4] << 8 | num[5]);

    data->temp = (int16_t)((uint16_t)num[6] << 8 | num[7]);
    data->Temp = 25 + ((double)data->temp) / 326.8;

    data->gx = (int16_t)((uint16_t)num[8] << 8 | num[9]);
    data->gy = (int16_t)((uint16_t)num[10] << 8 | num[11]);
    data->gz = (int16_t)((uint16_t)num[12] << 8 | num[13]);
    
    return 4;
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
