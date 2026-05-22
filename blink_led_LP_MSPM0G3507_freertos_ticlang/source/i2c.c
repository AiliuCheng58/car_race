#include "i2c.h"
#include "ti/driverlib/dl_gpio.h"
#include "ti/driverlib/dl_i2c.h"
#include "ti_msp_dl_config.h"

#define I2C_RECOVERY_PULSES        (9U)
#define I2C_RECOVERY_DELAY_CYCLES  (320U)

volatile uint8_t g_i2c_last_stage = 0;
volatile uint8_t g_i2c_last_addr = 0;
volatile uint8_t g_i2c_last_reg = 0;
volatile uint8_t g_i2c_last_len = 0;
volatile uint32_t g_i2c_last_status = 0;
volatile uint32_t g_i2c_last_rawint = 0;

static uint8_t s_i2c_current_addr = 0;
static uint8_t s_i2c_current_reg = 0;
static uint8_t s_i2c_current_len = 0;

static void i2c_set_current(uint8_t addr, uint8_t reg, uint8_t len)
{
    s_i2c_current_addr = addr;
    s_i2c_current_reg = reg;
    s_i2c_current_len = len;
}

static void i2c_record_stage(uint8_t stage)
{
    g_i2c_last_stage = stage;
    g_i2c_last_addr = s_i2c_current_addr;
    g_i2c_last_reg = s_i2c_current_reg;
    g_i2c_last_len = s_i2c_current_len;
    g_i2c_last_status = DL_I2C_getControllerStatus(I2C_1_INST);
    g_i2c_last_rawint = DL_I2C_getRawInterruptStatus(I2C_1_INST, 0xFFFFFFFFU);
}

static void i2c_short_delay(void)
{
    delay_cycles(I2C_RECOVERY_DELAY_CYCLES);
}

static bool i2c_sda_high(void)
{
    return (DL_GPIO_readPins(GPIO_I2C_1_SDA_PORT, GPIO_I2C_1_SDA_PIN) != 0U);
}

static bool i2c_scl_high(void)
{
    return (DL_GPIO_readPins(GPIO_I2C_1_SCL_PORT, GPIO_I2C_1_SCL_PIN) != 0U);
}

static void i2c_release_sda(void)
{
    DL_GPIO_disableOutput(GPIO_I2C_1_SDA_PORT, GPIO_I2C_1_SDA_PIN);
    DL_GPIO_initDigitalInputFeatures(GPIO_I2C_1_IOMUX_SDA,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
}

static void i2c_release_scl(void)
{
    DL_GPIO_disableOutput(GPIO_I2C_1_SCL_PORT, GPIO_I2C_1_SCL_PIN);
    DL_GPIO_initDigitalInputFeatures(GPIO_I2C_1_IOMUX_SCL,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_PULL_UP,
        DL_GPIO_HYSTERESIS_DISABLE, DL_GPIO_WAKEUP_DISABLE);
}

static void i2c_drive_sda_low(void)
{
    DL_GPIO_clearPins(GPIO_I2C_1_SDA_PORT, GPIO_I2C_1_SDA_PIN);
    DL_GPIO_initDigitalOutputFeatures(GPIO_I2C_1_IOMUX_SDA,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
        DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_enableOutput(GPIO_I2C_1_SDA_PORT, GPIO_I2C_1_SDA_PIN);
}

static void i2c_drive_scl_low(void)
{
    DL_GPIO_clearPins(GPIO_I2C_1_SCL_PORT, GPIO_I2C_1_SCL_PIN);
    DL_GPIO_initDigitalOutputFeatures(GPIO_I2C_1_IOMUX_SCL,
        DL_GPIO_INVERSION_DISABLE, DL_GPIO_RESISTOR_NONE,
        DL_GPIO_DRIVE_STRENGTH_LOW, DL_GPIO_HIZ_DISABLE);
    DL_GPIO_enableOutput(GPIO_I2C_1_SCL_PORT, GPIO_I2C_1_SCL_PIN);
}

static void i2c_restore_pins(void)
{
    DL_GPIO_disableOutput(GPIO_I2C_1_SDA_PORT, GPIO_I2C_1_SDA_PIN);
    DL_GPIO_disableOutput(GPIO_I2C_1_SCL_PORT, GPIO_I2C_1_SCL_PIN);

    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_I2C_1_IOMUX_SDA,
        GPIO_I2C_1_IOMUX_SDA_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_initPeripheralInputFunctionFeatures(GPIO_I2C_1_IOMUX_SCL,
        GPIO_I2C_1_IOMUX_SCL_FUNC, DL_GPIO_INVERSION_DISABLE,
        DL_GPIO_RESISTOR_NONE, DL_GPIO_HYSTERESIS_DISABLE,
        DL_GPIO_WAKEUP_DISABLE);
    DL_GPIO_enableHiZ(GPIO_I2C_1_IOMUX_SDA);
    DL_GPIO_enableHiZ(GPIO_I2C_1_IOMUX_SCL);
}

static void i2c_reset_transfer(void)
{
    DL_I2C_disableControllerReadOnTXEmpty(I2C_1_INST);
    DL_I2C_resetControllerTransfer(I2C_1_INST);
    DL_I2C_flushControllerRXFIFO(I2C_1_INST);
    DL_I2C_flushControllerTXFIFO(I2C_1_INST);
    DL_I2C_clearInterruptStatus(I2C_1_INST,
        DL_I2C_INTERRUPT_CONTROLLER_NACK |
        DL_I2C_INTERRUPT_CONTROLLER_RX_DONE |
        DL_I2C_INTERRUPT_CONTROLLER_TX_DONE |
        DL_I2C_INTERRUPT_CONTROLLER_RXFIFO_TRIGGER |
        DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER |
        DL_I2C_INTERRUPT_CONTROLLER_RXFIFO_FULL |
        DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_EMPTY |
        DL_I2C_INTERRUPT_CONTROLLER_START |
        DL_I2C_INTERRUPT_CONTROLLER_STOP |
        DL_I2C_INTERRUPT_CONTROLLER_ARBITRATION_LOST);
}

static bool i2c_recover_bus(void)
{
    bool recovered;

    DL_I2C_disableController(I2C_1_INST);
    i2c_reset_transfer();

    i2c_release_sda();
    i2c_release_scl();
    i2c_short_delay();

    for (uint8_t i = 0; (i < I2C_RECOVERY_PULSES) && !i2c_sda_high(); i++) {
        i2c_drive_scl_low();
        i2c_short_delay();
        i2c_release_scl();
        i2c_short_delay();

        if (!i2c_scl_high()) {
            break;
        }
    }

    i2c_drive_sda_low();
    i2c_short_delay();
    i2c_release_scl();
    i2c_short_delay();
    i2c_release_sda();
    i2c_short_delay();

    recovered = i2c_sda_high() && i2c_scl_high();

    i2c_restore_pins();
    SYSCFG_DL_I2C_1_init();
    i2c_reset_transfer();
    i2c_short_delay();

    return recovered;
}

static bool i2c_wait_ready(void)
{
    uint32_t timeout = I2C_TIMEOUT;
    uint8_t recovery_count = 0;

retry:
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY) {
        if (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
            i2c_record_stage(1U);
            i2c_reset_transfer();
            return false;
        }
        if (--timeout == 0U) {
            i2c_record_stage(2U);
            if ((recovery_count++ == 0U) && i2c_recover_bus()) {
                timeout = I2C_TIMEOUT;
                goto retry;
            }
            return false;
        }
    }

    if (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS) {
        i2c_record_stage(3U);
        if ((recovery_count++ == 0U) && i2c_recover_bus()) {
            timeout = I2C_TIMEOUT;
            goto retry;
        } else {
            return false;
        }
    }

    timeout = I2C_TIMEOUT;
    while (!(DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_IDLE)) {
        if (--timeout == 0U) {
            i2c_record_stage(4U);
            if ((recovery_count++ == 0U) && i2c_recover_bus()) {
                timeout = I2C_TIMEOUT;
                goto retry;
            }
            return false;
        }
    }

    return ((DL_I2C_getControllerStatus(I2C_1_INST) &
                DL_I2C_CONTROLLER_STATUS_BUSY_BUS) == 0U);
}

static bool i2c_wait_done(void)
{
    uint32_t timeout = I2C_TIMEOUT;

    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY) {
        if (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
            i2c_record_stage(5U);
            i2c_reset_transfer();
            return false;
        }
        if (--timeout == 0U) {
            i2c_record_stage(6U);
            i2c_recover_bus();
            return false;
        }
    }

    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS) {
        if (--timeout == 0U) {
            if (DL_I2C_getRawInterruptStatus(I2C_1_INST,
                    DL_I2C_INTERRUPT_CONTROLLER_TX_DONE |
                    DL_I2C_INTERRUPT_CONTROLLER_STOP) != 0U) {
                i2c_recover_bus();
                return true;
            }
            i2c_record_stage(7U);
            i2c_recover_bus();
            return false;
        }
    }

    if (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_ERROR) {
        i2c_record_stage(8U);
        i2c_reset_transfer();
        return false;
    }

    return true;
}

static bool i2c_wait_rx_data(void)
{
    uint32_t timeout = I2C_TIMEOUT;

    while (DL_I2C_isControllerRXFIFOEmpty(I2C_1_INST)) {
        uint32_t status = DL_I2C_getControllerStatus(I2C_1_INST);

        if ((status & DL_I2C_CONTROLLER_STATUS_IDLE) &&
            ((status & (DL_I2C_CONTROLLER_STATUS_BUSY |
                        DL_I2C_CONTROLLER_STATUS_BUSY_BUS)) == 0U)) {
            i2c_record_stage(9U);
            i2c_reset_transfer();
            return false;
        }

        if (--timeout == 0U) {
            i2c_record_stage(10U);
            i2c_recover_bus();
            return false;
        }
    }

    return true;
}

static bool i2c_wait_rx_done(void)
{
    uint32_t timeout = I2C_TIMEOUT;

    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY) {
        if (--timeout == 0U) {
            i2c_record_stage(11U);
            i2c_recover_bus();
            return false;
        }
    }

    timeout = I2C_TIMEOUT;
    while (DL_I2C_getControllerStatus(I2C_1_INST) & DL_I2C_CONTROLLER_STATUS_BUSY_BUS) {
        if (--timeout == 0U) {
            if (DL_I2C_getRawInterruptStatus(I2C_1_INST,
                    DL_I2C_INTERRUPT_CONTROLLER_RX_DONE |
                    DL_I2C_INTERRUPT_CONTROLLER_STOP) != 0U) {
                i2c_recover_bus();
                return true;
            }
            i2c_record_stage(12U);
            i2c_recover_bus();
            return false;
        }
    }

    i2c_reset_transfer();
    return true;
}

uint8_t i2c_SendByte(uint8_t slave_address, uint8_t reg_address, uint8_t data)
{
    uint8_t i2c_buf[2] = {reg_address, data};

    i2c_set_current(slave_address, reg_address, sizeof(i2c_buf));
    if (!i2c_wait_ready()) {
        return 0;
    }

    i2c_reset_transfer();
    if (DL_I2C_fillControllerTXFIFO(I2C_1_INST, i2c_buf, sizeof(i2c_buf)) !=
        sizeof(i2c_buf)) {
        i2c_record_stage(13U);
        return 0;
    }

    DL_I2C_disableInterrupt(I2C_1_INST, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
    DL_I2C_startControllerTransferAdvanced(I2C_1_INST, slave_address,
        DL_I2C_CONTROLLER_DIRECTION_TX, sizeof(i2c_buf),
        DL_I2C_CONTROLLER_START_ENABLE, DL_I2C_CONTROLLER_STOP_ENABLE,
        DL_I2C_CONTROLLER_ACK_ENABLE);

    return i2c_wait_done() ? 1U : 0U;
}

uint8_t i2c_SendBytes(uint8_t slave_address, uint8_t reg_address, uint8_t *data, uint8_t len)
{
    uint8_t i2c_buf[32];

    if ((data == NULL) || (len == 0U) || ((len + 1U) > sizeof(i2c_buf))) {
        i2c_record_stage(14U);
        return 0;
    }

    i2c_set_current(slave_address, reg_address, (uint8_t)(len + 1U));
    if (!i2c_wait_ready()) {
        return 0;
    }

    i2c_buf[0] = reg_address;
    for (uint8_t i = 0; i < len; i++) {
        i2c_buf[i + 1U] = data[i];
    }

    i2c_reset_transfer();
    if (DL_I2C_fillControllerTXFIFO(I2C_1_INST, i2c_buf, (uint16_t)(len + 1U)) !=
        (uint16_t)(len + 1U)) {
        i2c_record_stage(15U);
        return 0;
    }

    DL_I2C_disableInterrupt(I2C_1_INST, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
    DL_I2C_startControllerTransferAdvanced(I2C_1_INST, slave_address,
        DL_I2C_CONTROLLER_DIRECTION_TX, (uint16_t)(len + 1U),
        DL_I2C_CONTROLLER_START_ENABLE, DL_I2C_CONTROLLER_STOP_ENABLE,
        DL_I2C_CONTROLLER_ACK_ENABLE);

    return i2c_wait_done() ? 1U : 0U;
}

uint8_t i2c_ReadByte(uint8_t slave_address, uint8_t reg_address, uint8_t *data)
{
    return i2c_ReadBytes(slave_address, reg_address, data, 1U);
}

uint8_t i2c_ReadBytes(uint8_t slave_address, uint8_t reg_address, uint8_t *data, uint8_t len)
{
    DL_I2C_CONTROLLER_ACK ack;

    if ((data == NULL) || (len == 0U)) {
        i2c_record_stage(16U);
        return 0;
    }

    i2c_set_current(slave_address, reg_address, len);
    if (!i2c_wait_ready()) {
        return 0;
    }

    i2c_reset_transfer();
    if (DL_I2C_fillControllerTXFIFO(I2C_1_INST, &reg_address, 1U) != 1U) {
        i2c_record_stage(17U);
        return 0;
    }

    DL_I2C_disableInterrupt(I2C_1_INST, DL_I2C_INTERRUPT_CONTROLLER_TXFIFO_TRIGGER);
    DL_I2C_enableControllerReadOnTXEmpty(I2C_1_INST);
    ack = (len > 1U) ? DL_I2C_CONTROLLER_ACK_ENABLE : DL_I2C_CONTROLLER_ACK_DISABLE;
    DL_I2C_startControllerTransferAdvanced(I2C_1_INST, slave_address,
        DL_I2C_CONTROLLER_DIRECTION_RX, len, DL_I2C_CONTROLLER_START_ENABLE,
        DL_I2C_CONTROLLER_STOP_ENABLE, ack);

    for (uint8_t i = 0; i < len; i++) {
        if (!i2c_wait_rx_data()) {
            DL_I2C_disableControllerReadOnTXEmpty(I2C_1_INST);
            return 0;
        }
        data[i] = DL_I2C_receiveControllerData(I2C_1_INST);
    }

    DL_I2C_disableControllerReadOnTXEmpty(I2C_1_INST);
    return i2c_wait_rx_done() ? 1U : 0U;
}
