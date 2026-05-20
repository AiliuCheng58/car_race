#include "i.h"

#include "source/uart.h"
#include "ti/driverlib/dl_i2c.h"
#include "ti_msp_dl_config.h"

uint8_t i2c_SendByte(uint8_t slave_address, uint8_t reg_address, uint8_t data){
    uint32_t timeout;
    uint8_t i2c_buf[2];

    i2c_buf[0] = reg_address;
    i2c_buf[1] = data;

    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS){
        if (--timeout == 0)
            return 0;
    }

    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY){
        if (--timeout == 0)
            return 0;
    }

    DL_I2C_flushControllerTXFIFO(I2C_1_INST);

    if (DL_I2C_fillControllerTXFIFO(I2C_1_INST, i2c_buf, 2) != 2)
        return 0;

    DL_I2C_startControllerTransfer(I2C_1_INST, slave_address, DL_I2C_CONTROLLER_DIRECTION_TX, 2);

    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(I2C_1_INST) & (DL_I2C_CONTROLLER_STATUS_BUSY | DL_I2C_CONTROLLER_STATUS_BUSY_BUS)){
        if (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_ERROR)
            return 0;

        if (--timeout == 0)
            return 0;
    }

    return 1;
}

uint8_t i2c_SendBytes(uint8_t slave_address, uint8_t reg_address, uint8_t *data, uint8_t len){
    uint8_t i;
    uint8_t i2c_buf[32];
    uint32_t timeout;

    if (data == NULL || len == 0)
        return 0;

    if (len + 1 > 32)
        return 0;

    i2c_buf[0] = reg_address;

    for (i = 0; i < len; i++)
        i2c_buf[i + 1] = data[i];

    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS) {
        if (--timeout == 0)
            return 0;
    }

    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY) {
        if (--timeout == 0)
            return 0;
    }

    DL_I2C_flushControllerTXFIFO(I2C_1_INST);

    if (DL_I2C_fillControllerTXFIFO(I2C_1_INST, i2c_buf, len + 1) != len + 1)
        return 0;

    DL_I2C_startControllerTransfer(I2C_1_INST, slave_address, DL_I2C_CONTROLLER_DIRECTION_TX, len + 1);

    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(I2C_1_INST) &
          (DL_I2C_CONTROLLER_STATUS_BUSY | DL_I2C_CONTROLLER_STATUS_BUSY_BUS)) {
        if (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_ERROR)
            return 0;

        if (--timeout == 0)
            return 0;
    }

    return 1;
}

uint8_t i2c_ReadByte(uint8_t slave_address, uint8_t reg_address, uint8_t *data)
{
    uint32_t timeout;
    uint32_t status;
    uart_SendByte(3);
    if (data == NULL) return 0;

    while (DL_I2C_getControllerStatus(I2C_1_INST) &
           DL_I2C_CONTROLLER_STATUS_BUSY_BUS);

    DL_I2C_flushControllerRXFIFO(I2C_1_INST);
    DL_I2C_flushControllerTXFIFO(I2C_1_INST);
    
    // 写寄存器地址，不发 STOP
    DL_I2C_transmitControllerData(I2C_1_INST, reg_address);
    uart_SendByte(4);
    DL_I2C_startControllerTransferAdvanced(
        I2C_1_INST,
        slave_address,
        DL_I2C_CONTROLLER_DIRECTION_TX,
        1,
        DL_I2C_CONTROLLER_START_ENABLE,
        DL_I2C_CONTROLLER_STOP_DISABLE,
        DL_I2C_CONTROLLER_ACK_ENABLE
    );
    uart_SendByte(5);
    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(I2C_1_INST) &
           DL_I2C_CONTROLLER_STATUS_BUSY) {
        status = DL_I2C_getControllerStatus(I2C_1_INST);

        if (status & DL_I2C_CONTROLLER_STATUS_ERROR) {
            uart_SendString("tx_error\r\n");
            return 0;
        }

        if (--timeout == 0) {
            uart_SendString("tx_timeout\r\n");
            return 0;
        }
    }

    // 读 1 字节，发 repeated start，发 STOP
    DL_I2C_startControllerTransferAdvanced(
        I2C_1_INST,
        slave_address,
        DL_I2C_CONTROLLER_DIRECTION_RX,
        1,
        DL_I2C_CONTROLLER_START_ENABLE,
        DL_I2C_CONTROLLER_STOP_ENABLE,
        DL_I2C_CONTROLLER_ACK_ENABLE
    );

    timeout = I2C_TIMEOUT;
    while (DL_I2C_isControllerRXFIFOEmpty(I2C_1_INST)) {
        status = DL_I2C_getControllerStatus(I2C_1_INST);

        if (status & DL_I2C_CONTROLLER_STATUS_ERROR) {
            uart_SendString("rx_error\r\n");
            return 0;
        }

        if (--timeout == 0) {
            uart_SendString("rx_timeout\r\n");
            return 0;
        }
    }

    *data = DL_I2C_receiveControllerData(I2C_1_INST);

    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(I2C_1_INST) &
           (DL_I2C_CONTROLLER_STATUS_BUSY |
            DL_I2C_CONTROLLER_STATUS_BUSY_BUS)) {
        if (--timeout == 0) {
            uart_SendString("end_timeout\r\n");
            return 0;
        }
    }

    return 1;
}

uint8_t i2c_ReadBytes(uint8_t slave_address, uint8_t reg_address, uint8_t *data, uint8_t len){
    uint32_t timeout;
    uint8_t i = 0;

    if (data == NULL || len == 0)
        return 0;

    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS) {
        if (--timeout == 0)
            return 0;
    }

    DL_I2C_flushControllerRXFIFO(I2C_1_INST);
    DL_I2C_flushControllerTXFIFO(I2C_1_INST);

    if (DL_I2C_fillControllerTXFIFO(I2C_1_INST, &reg_address, 1) != 1)
        return 0;

    DL_I2C_startControllerTransfer(I2C_1_INST, slave_address, DL_I2C_CONTROLLER_DIRECTION_TX, 1);

    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(I2C_1_INST) & (DL_I2C_CONTROLLER_STATUS_BUSY | DL_I2C_CONTROLLER_STATUS_BUSY_BUS)) {
        if (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_ERROR)
            return 0;
        if (--timeout == 0)
            return 0;
    }

    if (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_ERROR)
        return 0;

    DL_I2C_startControllerTransfer(I2C_1_INST, slave_address, DL_I2C_CONTROLLER_DIRECTION_RX, len);

    timeout = I2C_TIMEOUT;
    while (i < len) {
        if (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_ERROR)
            return 0;

        if (!DL_I2C_isControllerRXFIFOEmpty(I2C_1_INST)) {
            data[i++] = DL_I2C_receiveControllerData(I2C_1_INST);
            timeout = I2C_TIMEOUT;
        } else {
            if (--timeout == 0)
                return 0;
        }
    }

    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(I2C_1_INST) & (DL_I2C_CONTROLLER_STATUS_BUSY | DL_I2C_CONTROLLER_STATUS_BUSY_BUS)) {
        if (--timeout == 0)
            return 0;
    }

    if (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_ERROR)
        return 0;

    return 1;
}
