#include "i2c.h"
#include "projdefs.h"
#include "ti/driverlib/dl_i2c.h"
#include "ti/driverlib/dl_uart.h"
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

    i2c_buf[0] = reg_address;
    i2c_buf[1] = data;

    // 清空 FIFO
    DL_I2C_flushControllerRXFIFO(I2C_1_INST);
    DL_I2C_flushControllerTXFIFO(I2C_1_INST);
    DL_I2C_fillControllerTXFIFO(I2C_1_INST, i2c_buf, 2);
    // 禁用I2C传输触发中断  重要
    DL_I2C_disableInterrupt(I2C_1_INST, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);

    //等i2c控制器空闲
    while (!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
    DL_I2C_startControllerTransferAdvanced(I2C_1_INST, slave_address, DL_I2C_CONTROLLER_DIRECTION_TX, 2, DL_I2C_CONTROLLER_START_ENABLE, DL_I2C_CONTROLLER_STOP_ENABLE, DL_I2C_CONTROLLER_ACK_ENABLE);

    //等总线
    while(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS);
    //等控制器闲
    while(!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));

    return 1;
}

/**
 * @brief I2C 连续写入多个字节数据
 * @param slave_address  从机地址
 * @param reg_address    起始寄存器地址
 * @param data           数据数组指针
 * @param len            要发送的数据长度 (不含寄存器地址)
 */
uint8_t i2c_SendBytes(uint8_t slave_address, uint8_t reg_address, uint8_t *data, uint8_t len) {
    uint8_t i2c_buf[32];

    i2c_buf[0] = reg_address;
    for (uint8_t i = 0; i < len; i++) {
        i2c_buf[i + 1] = data[i];
    }

    // 清空 FIFO
    DL_I2C_flushControllerRXFIFO(I2C_1_INST);
    DL_I2C_flushControllerTXFIFO(I2C_1_INST);
    DL_I2C_fillControllerTXFIFO(I2C_1_INST, i2c_buf, (len + 1));
    // 禁用I2C传输触发中断  重要
    DL_I2C_disableInterrupt(I2C_1_INST, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);

    //等i2c控制器空闲
    while (!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
    DL_I2C_startControllerTransferAdvanced(I2C_1_INST, slave_address, DL_I2C_CONTROLLER_DIRECTION_TX, len + 1, DL_I2C_CONTROLLER_START_ENABLE, DL_I2C_CONTROLLER_STOP_ENABLE, DL_I2C_CONTROLLER_ACK_ENABLE);

    //等总线
    while(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS);
    //等控制器闲
    while(!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));

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
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS)
        DL_UART_transmitDataBlocking(UART_0_INST, 'Z');
    // 清空 FIFO
    DL_I2C_flushControllerRXFIFO(I2C_1_INST);
    DL_I2C_flushControllerTXFIFO(I2C_1_INST);
    DL_I2C_fillControllerTXFIFO(I2C_1_INST, &reg_address, 1);
    DL_UART_transmitDataBlocking(UART_0_INST, 'C');
    // 禁用I2C传输触发中断  重要
    DL_I2C_disableInterrupt(I2C_1_INST, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
    //等i2c控制器空闲
    while(!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
    DL_I2C_enableControllerReadOnTXEmpty(I2C_1_INST);
    DL_I2C_startControllerTransferAdvanced(I2C_1_INST,
                                        slave_address,
                                        DL_I2C_CONTROLLER_DIRECTION_RX,
                                        1,
                                        DL_I2C_CONTROLLER_START_ENABLE,
                                        DL_I2C_CONTROLLER_STOP_ENABLE,
                                        DL_I2C_CONTROLLER_ACK_DISABLE
    );
    DL_UART_transmitDataBlocking(UART_0_INST, 'D');
    
    //等读到
    while (DL_I2C_isControllerRXFIFOEmpty(I2C_1_INST)) {
        if (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
            DL_UART_transmitDataBlocking(UART_0_INST, 'X');
            return 0;
        }
        if (DL_I2C_getRawInterruptStatus(I2C_1_INST, DL_I2C_INTERRUPT_CONTROLLER_NACK)) {
            DL_UART_transmitDataBlocking(UART_0_INST, 'N');
            return 0;
        }
    }
    DL_UART_transmitDataBlocking(UART_0_INST, 'E');
    *data = DL_I2C_receiveControllerData(I2C_1_INST);
    DL_I2C_disableControllerReadOnTXEmpty(I2C_1_INST);
    return 1;
}

/**
 * @brief I2C 连续读取多个字节数据 (标准的“写地址+读数据”流程)
 * @param slave_address  从机地址
 * @param reg_address    起始寄存器地址
 * @param data           存储读取结果的数组指针
 * @param len            要读取的字节数
 */
uint8_t i2c_ReadBytes(uint8_t slave_address, uint8_t reg_address, uint8_t *data, uint8_t len)
{
    uint8_t i = 0;
    // 清空 FIFO
    DL_I2C_flushControllerRXFIFO(I2C_1_INST);
    DL_I2C_flushControllerTXFIFO(I2C_1_INST);
    // 填入要读的寄存器地址
    DL_I2C_fillControllerTXFIFO(I2C_1_INST, &reg_address, 1);
    // 禁用 TX FIFO 触发中断
    DL_I2C_disableInterrupt(I2C_1_INST, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
    // 等控制器空闲
    while(!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
    DL_I2C_startControllerTransferAdvanced(I2C_1_INST, slave_address, DL_I2C_CONTROLLER_DIRECTION_TX, 1, DL_I2C_CONTROLLER_START_ENABLE, DL_I2C_CONTROLLER_STOP_DISABLE, DL_I2C_CONTROLLER_ACK_ENABLE);
    // 等总线空闲
    //while(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS);
    // 等控制器空闲
    //while(!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE));
    //while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY);
    // 启动接收 len 个字节
    DL_I2C_startControllerTransferAdvanced(I2C_1_INST, slave_address, DL_I2C_CONTROLLER_DIRECTION_RX, len, DL_I2C_CONTROLLER_START_ENABLE, DL_I2C_CONTROLLER_STOP_ENABLE, DL_I2C_CONTROLLER_ACK_DISABLE);
    // 逐字节读取
    while(i < len){
        while(DL_I2C_isControllerRXFIFOEmpty(I2C_1_INST));
        data[i++] = DL_I2C_receiveControllerData(I2C_1_INST);
    }
    return 1;
}
