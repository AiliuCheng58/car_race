#include "i2c.h"
#include "ti/driverlib/dl_i2c.h"
#include "ti_msp_dl_config.h"

/**
 * @brief I2C 向指定寄存器写入单字节数据
 * @param slave_address  从机 7 位地址 (如 0x68)
 * @param reg_address    寄存器地址
 * @param data           要写入的数据
 * @return uint8_t       1: 成功, 0: 失败/超时
 */
uint8_t i2c_SendByte(uint8_t slave_address, uint8_t reg_address, uint8_t data) {
    uint8_t i2c_buf[2];
    i2c_buf[0] = reg_address; // 第一个字节是寄存器地址
    i2c_buf[1] = data;        // 第二个字节是数据内容
    // 等待总线和控制器空闲
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS);
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE);

    // 清理发送 FIFO 并装载数据
    DL_I2C_flushControllerTXFIFO(I2C_1_INST);
    DL_I2C_fillControllerTXFIFO(I2C_1_INST, i2c_buf, 2);
    // 开始发送
    DL_I2C_startControllerTransfer(I2C_1_INST, slave_address, DL_I2C_CONTROLLER_DIRECTION_TX, 2);
    // 等总线和控制器
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS);
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE);

    return 1;
}

/**
 * @brief I2C 连续写入多个字节数据 (常用于初始化或突发写)
 * @param slave_address  从机地址
 * @param reg_address    起始寄存器地址
 * @param data           数据数组指针
 * @param len            要发送的数据长度 (不含寄存器地址)
 */
uint8_t i2c_SendBytes(uint8_t slave_address, uint8_t reg_address, uint8_t *data, uint8_t len) {
    uint8_t i2c_buf[32]; // 临时缓冲区，上限需与 len 匹配
    i2c_buf[0] = reg_address;
    for (uint8_t i = 0; i < len; i++) {
        i2c_buf[i + 1] = data[i]; // 将寄存器地址和数据拼接在同一个包里
    }
    // 等总线和控制器
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS);
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE);
    // 清理并填充 TX FIFO
    DL_I2C_flushControllerTXFIFO(I2C_1_INST);
    DL_I2C_fillControllerTXFIFO(I2C_1_INST, i2c_buf, (len + 1));
    // 开始发送
    DL_I2C_startControllerTransfer(I2C_1_INST, slave_address, DL_I2C_CONTROLLER_DIRECTION_TX, len + 1);
    // 等总线和控制器
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS);
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE);

    return 1;
}

/**
 * @brief I2C 读取单字节数据
 * @param slave_address  从机 7 位地址
 * @param reg_address    要读取的寄存器地址
 * @param data           存储读取结果的指针
 * @return uint8_t       1: 成功, 0: 失败
 */
uint8_t i2c_ReadByte(uint8_t slave_address, uint8_t reg_address, uint8_t *data){
    DL_I2C_fillControllerTXFIFO(I2C_1_INST, &reg_address, 1);
    //禁用中断
    DL_I2C_disableInterrupt(I2C_1_INST, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
    //等i2c控制器空闲
    while(!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
    //启动发送
    DL_I2C_startControllerTransfer(I2C_1_INST, slave_address, DL_I2C_CONTROLLER_DIRECTION_TX, 1);
    //等总线
    //while(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS);
    //等控制器闲
    while(!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
    //启动接收
    DL_I2C_startControllerTransfer(I2C_1_INST, slave_address, DL_I2C_CONTROLLER_DIRECTION_RX, 1);
    //等总线
    while(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS);
    //等控制器闲
    while(!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
    *data = DL_I2C_receiveControllerData(I2C_1_INST);
    return 1;
}

/**
 * @brief I2C 连续读取多个字节数据 (标准的“写地址+读数据”流程)
 * @param slave_address  从机地址
 * @param reg_address    起始寄存器地址
 * @param data           存储读取结果的数组指针
 * @param len            要读取的字节数
 */
uint8_t i2c_ReadBytes(uint8_t slave_address, uint8_t reg_address, uint8_t *data, uint8_t len) {
    uint8_t i = 0;
    // 等总线空闲
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS);
    // 清空缓存+填充缓存
    DL_I2C_flushControllerRXFIFO(I2C_1_INST);
    DL_I2C_flushControllerTXFIFO(I2C_1_INST);
    DL_I2C_fillControllerTXFIFO(I2C_1_INST, &reg_address, 1);
    //等i2c控制器空闲
    while(!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
    // 开始发送
    DL_I2C_startControllerTransfer(I2C_1_INST, slave_address, DL_I2C_CONTROLLER_DIRECTION_TX, 1);
    // 等总线空闲
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS);
    //等i2c控制器空闲
    while(!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
    // 开始接收
    DL_I2C_startControllerTransfer(I2C_1_INST, slave_address, DL_I2C_CONTROLLER_DIRECTION_RX, len);
    while (i < len)
        if (!DL_I2C_isControllerRXFIFOEmpty(I2C_1_INST))
            data[i++] = DL_I2C_receiveControllerData(I2C_1_INST);
    // 等总线空闲
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS);
    //等i2c控制器空闲
    while(!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
    return 1;
}