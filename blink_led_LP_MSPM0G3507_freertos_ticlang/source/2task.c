#include "2task.h"
#include "Driver/icm20608.h"
#include "ti/driverlib/dl_uart.h"

void LED(void *pvParameters){
    (void) pvParameters;
    while (1) {
        // s5 -> bit7
        if ((track >> 7) & 1)   DL_GPIO_clearPins(GPIO_LEDS_s5_PORT, GPIO_LEDS_s5_PIN);
        else                    DL_GPIO_setPins(GPIO_LEDS_s5_PORT, GPIO_LEDS_s5_PIN);
        // s6 -> bit6
        if ((track >> 6) & 1)   DL_GPIO_clearPins(GPIO_LEDS_s6_PORT, GPIO_LEDS_s6_PIN);
        else                    DL_GPIO_setPins(GPIO_LEDS_s6_PORT, GPIO_LEDS_s6_PIN);
        // s7 -> bit5
        if ((track >> 5) & 1)   DL_GPIO_clearPins(GPIO_LEDS_s7_PORT, GPIO_LEDS_s7_PIN);
        else                    DL_GPIO_setPins(GPIO_LEDS_s7_PORT, GPIO_LEDS_s7_PIN);
        // s8 -> bit4
        if ((track >> 4) & 1)   DL_GPIO_clearPins(GPIO_LEDS_s8_PORT, GPIO_LEDS_s8_PIN);
        else                    DL_GPIO_setPins(GPIO_LEDS_s8_PORT, GPIO_LEDS_s8_PIN);
        // s4 -> bit3
        if ((track >> 3) & 1)   DL_GPIO_clearPins(GPIO_LEDS_s4_PORT, GPIO_LEDS_s4_PIN);
        else                    DL_GPIO_setPins(GPIO_LEDS_s4_PORT, GPIO_LEDS_s4_PIN);
        // s3 -> bit2
        if ((track >> 2) & 1)   DL_GPIO_clearPins(GPIO_LEDS_s3_PORT, GPIO_LEDS_s3_PIN);
        else                    DL_GPIO_setPins(GPIO_LEDS_s3_PORT, GPIO_LEDS_s3_PIN);
        // s2 -> bit1
        if ((track >> 1) & 1)   DL_GPIO_clearPins(GPIO_LEDS_s2_PORT, GPIO_LEDS_s2_PIN);
        else                    DL_GPIO_setPins(GPIO_LEDS_s2_PORT, GPIO_LEDS_s2_PIN);
        // s1 -> bit0
        if ((track >> 0) & 1)   DL_GPIO_clearPins(GPIO_LEDS_s1_PORT, GPIO_LEDS_s1_PIN);
        else                    DL_GPIO_setPins(GPIO_LEDS_s1_PORT, GPIO_LEDS_s1_PIN);
        delay_ms(10);
    }
}
/*
void UART(void *pvParameters){
    (void) pvParameters;

    static char uart_buf[128];

    while(1)
    {
        uint16_t adc0_local[4];
        uint16_t adc1_local[4];
        uint8_t  track_local;
        bool     both_ready = false;
        taskENTER_CRITICAL();
        if (ready_0 && ready_1)
        {
            both_ready = true;
            ready_0 = false;
            ready_1 = false;

            adc0_local[0] = adc0[0];
            adc0_local[1] = adc0[1];
            adc0_local[2] = adc0[2];
            adc0_local[3] = adc0[3];

            adc1_local[0] = adc1[0];
            adc1_local[1] = adc1[1];
            adc1_local[2] = adc1[2];
            adc1_local[3] = adc1[3];

            track_local = track;
        }
        taskEXIT_CRITICAL();

        snprintf(uart_buf, sizeof(uart_buf),
                    "s1:%d\r\n"
                    "s2:%d\r\n"
                    "s3:%d\r\n"
                    "s4:%d\r\n"
                    "s5:%d\r\n"
                    "s6:%d\r\n"
                    "s7:%d\r\n"
                    "s8:%d\r\n"
                    "track:%d%d%d%d%d%d%d%d\r\n",
                    adc0_local[0],
                    adc0_local[1],
                    adc0_local[2],
                    adc0_local[3],
                    adc1_local[0],
                    adc1_local[1],
                    adc1_local[2],
                    adc1_local[3],
                    (track_local >> 7) & 1,
                    (track_local >> 6) & 1,
                    (track_local >> 5) & 1,
                    (track_local >> 4) & 1,
                    (track_local >> 3) & 1,
                    (track_local >> 2) & 1,
                    (track_local >> 1) & 1,
                    (track_local >> 0) & 1);
                    
            char *p = uart_buf;
            while (*p != '\0'){
                DL_UART_transmitDataBlocking(UART_0_INST, *p);
                p++;
            }

        DL_GPIO_togglePins(GPIO_LEDS_RUN_LED_PORT, GPIO_LEDS_RUN_LED_PIN);
        delay_ms(50);
    }
}
*/
void UART(void *pvParameters){
    (void) pvParameters;

    while(1)
    {
        uint8_t  track_local;
        bool     both_ready = false;
        taskENTER_CRITICAL();
        if (ready_0 && ready_1)
        {
            both_ready = true;
            ready_0 = false;
            ready_1 = false;

            track_local = track;
        }
        taskEXIT_CRITICAL();

        if(both_ready){
            DL_UART_transmitDataBlocking(UART_0_INST, 0xFE);
            DL_UART_transmitDataBlocking(UART_0_INST, track_local);
            DL_UART_transmitDataBlocking(UART_0_INST, 0xEF);
        }
        DL_GPIO_togglePins(GPIO_LEDS_RUN_LED_PORT, GPIO_LEDS_RUN_LED_PIN);
        delay_ms(50);
    }
}

void ICM(void *pvParameters){
    (void) pvParameters;
    
    ICM_Data imu_temp;
    uint8_t read_result;

    while(1)
    {
        if (g_imu_init_status != 10U)
        {
            taskENTER_CRITICAL();
            g_imu_ready = 0;
            g_imu_read_status = 3;
            taskEXIT_CRITICAL();

            (void) icm_init();
            delay_ms(200);
            continue;
        }

        read_result = icm_ReadRaw(&imu_temp);
        if(read_result)
        {
            taskENTER_CRITICAL();
            g_imu_raw = imu_temp;
            g_imu_ready = 1;
            g_imu_read_status = read_result;
            g_imu_sample_count++;
            taskEXIT_CRITICAL();
        }
        else
        {
            taskENTER_CRITICAL();
            g_imu_ready = 0;
            g_imu_read_status = 2;
            taskEXIT_CRITICAL();
        }

        delay_ms(50);   // 20Hz采集
    }
}
