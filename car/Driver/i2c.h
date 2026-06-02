#ifndef __I2C_H_
#define __I2C_H_ // 防止 i2c.h 被重复包含

#include <stdint.h>

#define I2C_TIMEOUT (100000U) // 单次 I2C 等待超时计数，超时后尝试恢复总线

extern volatile uint8_t g_i2c_last_stage; // 最近一次 I2C 异常发生的阶段，调试时查看
extern volatile uint8_t g_i2c_last_addr; // 最近一次访问的从机地址
extern volatile uint8_t g_i2c_last_reg; // 最近一次访问的寄存器地址
extern volatile uint8_t g_i2c_last_len; // 最近一次访问的数据长度
extern volatile uint32_t g_i2c_last_status; // 最近一次异常时的 I2C 状态寄存器
extern volatile uint32_t g_i2c_last_rawint; // 最近一次异常时的 I2C 原始中断状态

uint8_t i2c_SendByte(uint8_t slave_address, uint8_t reg_address,
    uint8_t data); // 向指定寄存器写入一个字节
uint8_t i2c_SendBytes(uint8_t slave_address, uint8_t reg_address,
    uint8_t *data, uint8_t len); // 从指定寄存器开始连续写入多个字节
uint8_t i2c_ReadByte(uint8_t slave_address, uint8_t reg_address,
    uint8_t *data); // 从指定寄存器读取一个字节
uint8_t i2c_ReadBytes(uint8_t slave_address, uint8_t reg_address,
    uint8_t *data, uint8_t len); // 从指定寄存器开始连续读取多个字节

#endif
