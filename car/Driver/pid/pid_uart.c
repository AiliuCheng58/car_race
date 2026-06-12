#include "pid.h"
#include "pid_internal.h"

#include "uart/uart.h"
#include "ti_msp_dl_config.h"

#include <stddef.h>
#include <stdlib.h>

#define PID_UART_LINE_MAX_LEN (96U) // 单条串口调参文本的最大缓存长度

void pid_read(PID *left_pid, PID *right_pid)
{
    if ((left_pid == NULL) && (right_pid == NULL)) {
        return;
    }

    char line[PID_UART_LINE_MAX_LEN]; // 保存串口收到的一整行文本
    uint16_t line_end = 0U;           // 指向 CR 或 LF 的位置
    uint16_t copy_len;                // 实际复制到 line 的字符数
    uint16_t consume_len;             // 需要从 UART 缓存移除的字符数
    uint32_t primask;                 // 保存进入函数前的中断状态

    float kp = 0.0f;
    float ki = 0.0f;
    float kd = 0.0f;
    uint8_t has_kp = 0U;
    uint8_t has_ki = 0U;
    uint8_t has_kd = 0U;

    primask = __get_PRIMASK();
    __disable_irq();

    while (line_end < RX_index) {
        if ((UART_Data[line_end] == '\n') || (UART_Data[line_end] == '\r')) {
            break;
        }
        line_end++;
    }

    if (line_end >= RX_index) { // 还没有完整一行
        __set_PRIMASK(primask);
        return;
    }

    copy_len = line_end;
    if (copy_len >= sizeof(line)) {
        copy_len = sizeof(line) - 1U;
    }

    for (uint16_t i = 0U; i < copy_len; i++) {
        line[i] = (char) UART_Data[i];
    }
    line[copy_len] = '\0';

    consume_len = line_end + 1U;
    while ((consume_len < RX_index) &&
        ((UART_Data[consume_len] == '\n') || (UART_Data[consume_len] == '\r'))) {
        consume_len++;
    }

    __set_PRIMASK(primask);

    char *text = line;

    // 跳过行首可能混入的无关字节，找到 kp/ki/kd 的 k
    while (*text != '\0') {
        char ch = *text;
        if ((ch >= 'A') && (ch <= 'Z')) {
            ch = (char) (ch + ('a' - 'A'));
        }

        if (ch == 'k') {
            break;
        }

        text++;
    }

    // 这一行不是 PID 调参文本，不消费缓存，留给循迹帧解析
    if (*text == '\0') {
        return;
    }

    // 确认是 PID 调参文本后，移除这一整行
    UART_MoveBytes(consume_len);

    while (*text != '\0') {
        while ((*text == ' ') || (*text == '\t') || (*text == ',')) {
            text++;
        }

        if (*text == '\0') {
            break;
        }

        char k = text[0];
        char item = text[1];

        if ((k >= 'A') && (k <= 'Z')) {
            k = (char) (k + ('a' - 'A'));
        }
        if ((item >= 'A') && (item <= 'Z')) {
            item = (char) (item + ('a' - 'A'));
        }

        if ((k != 'k') || ((item != 'p') && (item != 'i') && (item != 'd'))) {
            UART_SendString("PID ERR: use kp=1.0, ki=0.1, kd=0.0\r\n");
            return;
        }

        text += 2;

        while ((*text == ' ') || (*text == '\t')) {
            text++;
        }

        if (*text != '=') {
            UART_SendString("PID ERR: use kp=1.0, ki=0.1, kd=0.0\r\n");
            return;
        }

        text++;

        while ((*text == ' ') || (*text == '\t')) {
            text++;
        }

        char *end = text;
        float value = strtof(text, &end);

        if (end == text) {
            UART_SendString("PID ERR: use kp=1.0, ki=0.1, kd=0.0\r\n");
            return;
        }

        if (item == 'p') {
            kp = value;
            has_kp = 1U;
        } else if (item == 'i') {
            ki = value;
            has_ki = 1U;
        } else {
            kd = value;
            has_kd = 1U;
        }

        text = end;

        while ((*text == ' ') || (*text == '\t')) {
            text++;
        }

        if ((*text != ',') && (*text != '\0')) {
            UART_SendString("PID ERR: use kp=1.0, ki=0.1, kd=0.0\r\n");
            return;
        }
    }

    if ((has_kp == 0U) && (has_ki == 0U) && (has_kd == 0U)) {
        UART_SendString("PID ERR: use kp=1.0, ki=0.1, kd=0.0\r\n");
        return;
    }

    primask = __get_PRIMASK();
    __disable_irq();
    pid_write_params(left_pid, has_kp, kp, has_ki, ki, has_kd, kd);
    pid_write_params(right_pid, has_kp, kp, has_ki, ki, has_kd, kd);
    __set_PRIMASK(primask);

    UART_SendString("PID OK\r\n");
}
