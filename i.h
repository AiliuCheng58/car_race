#ifndef __I2C_H_
#define __I2C_H_

#include "head.h"

#define I2C_TIMEOUT 100000

uint8_t i2c_SendByte(uint8_t slave_address, uint8_t reg_address, uint8_t data);
uint8_t i2c_SendBytes(uint8_t slave_address, uint8_t reg_address, uint8_t *data, uint8_t len);
uint8_t i2c_ReadByte(uint8_t slave_address, uint8_t reg_address, uint8_t *data);
uint8_t i2c_ReadBytes(uint8_t slave_address, uint8_t reg_address, uint8_t *data, uint8_t len);

#endif
