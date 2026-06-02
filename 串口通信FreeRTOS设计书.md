# blink_led ↔ car 串口通信 — FreeRTOS 多任务与同步设计书

> 版本: v1.0  
> 平台: TI MSPM0G3507 (Cortex-M0+) + FreeRTOS V11.2.0  
> 原则: 仅扩展，不破坏现有代码结构  

---

## 一、总体架构

```
┌── blink_led (传感器采集端) ──────────────┐      UART 串口       ┌── car (控制执行端) ──────────────────┐
│                                         │ ◄══════════════════► │                                       │
│  ADC_ISR ──通知──► Track_Task           │   帧协议 (115200)     │  UART_ISR ──通知──► UART_RX_Task      │
│                           │             │                      │                           │             │
│  ICM_Task (50ms周期) ────┤              │                      │                     Mutex保护共享数据    │
│                      │    │             │                      │                           │             │
│                      ▼    ▼             │                      │                           ▼             │
│                 EventGroup              │                      │  Timer_ISR ──Queue──► PID_Task            │
│               (IMU_BIT|TRACK_BIT)       │                      │                (10ms周期)               │
│                      │                  │                      │                    │                    │
│                      ▼                  │                      │               motor_set()               │
│                UART_TX_Task             │                      │                                         │
│                 打包帧 → DMA发送         │                      │  Heartbeat_Task (1s通信超时安全刹车)    │
└─────────────────────────────────────────┘                      └─────────────────────────────────────────┘
```

---

## 二、blink_led 端设计（传感器采集 + 数据上报）

### 2.1 现有基础（不动）

| 文件 | 内容 | 状态 |
|------|------|------|
| `source/2task.c` | `LED_Task`、`ICM_Task`、`UART_Task` 三个任务函数体 | 保留，在此基础上扩展 |
| `source/app_main.c` | `app_main()` 创建任务、启动调度器 | 新增任务创建 + EventGroup 创建 |
| `Driver/icm20608.c` | IMU 驱动 | 不动 |
| `Driver/track.c` | ADC ISR 采集循迹数据 | ISR 末尾新增通知 |
| `source/i2c.c` | I2C 驱动 | 不动 |
| `source/uart.c` | UART 阻塞发送 | 改为 DMA/中断发送（保留原函数兼容） |

### 2.2 任务扩展方案

保持现有 3 个任务不变，新增：

| 任务 | 优先级 | 建议栈 | 周期 | 说明 |
|------|:---:|------|------|------|
| **ICM_Task**（现有） | 2 | 不变 | 50ms | 采集 IMU 数据到 `g_imu_raw`，采集完成后设 EventGroup 位 |
| **LED_Task**（现有） | 1 | 不变 | 10ms | 循迹 LED 指示，**不变** |
| **Track_Task**（新增） | 2 | 384B | 事件驱动 | 被 ADC ISR 通过任务通知唤醒，处理 `adc0[]/adc1[]` 得到 `g_track_data`，设 EventGroup 位 |
| **UART_TX_Task**（重构现有 UART_Task） | 2 | 640B+ | 事件驱动 | 等待 EventGroup（IMU+TRAK 全就绪）→ 打包帧 → 发送 |

> **为什么新增 Track_Task？**  
> 当前 ADC ISR 只设 `ready_0/ready_1` 标志，UART_Task 轮询 50ms 后才处理。  
> 新增 Track_Task 让 ISR 直接唤醒它处理循迹数据，UART_TX_Task 只管打包发送，职责清晰。

### 2.3 同步机制

| 机制 | 数量 | 发送方 | 接收方 | 用途 |
|------|:---:|--------|--------|------|
| **EventGroup** | 1 | ICM_Task / Track_Task | UART_TX_Task | `xEventGroupSetBits` 设位 → `xEventGroupWaitBits` 等全部就绪 |
| **Task Notification** | 1 | ADC_ISR | Track_Task | ISR 中 `vTaskNotifyGiveFromISR` → 任务中 `ulTaskNotifyTake` |

### 2.4 EventGroup 位定义

```c
#define IMU_READY_BIT    (1 << 0)   // ICM_Task 采集完毕
#define TRACK_READY_BIT  (1 << 1)   // Track_Task 处理完毕
#define ALL_SENSOR_BITS  (IMU_READY_BIT | TRACK_READY_BIT)
```

### 2.5 任务/ISR 伪代码

```c
// ========== ICM_Task（现有基础上增加一行）==========
void ICM(void *pv) {
    // ... 现有初始化代码不变 ...
    for (;;) {
        // ... 现有 I2C 读取 IMU 代码不变 ...
        // g_imu_raw 已填充

        xEventGroupSetBits(sensor_ready_evt, IMU_READY_BIT);  // ← 新增：通知数据就绪

        delay_ms(50);  // 现有延时
    }
}

// ========== ADC ISR（现有 track.c 基础上增加通知）==========
void ADC12_0_INST_IRQHandler(void) {
    // ... 现有 ADC 读取代码不变 ...
    // ready_0 = true 等标志不变

    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    vTaskNotifyGiveFromISR(Track_Task_Handle, &xHigherPriorityTaskWoken);  // ← 新增
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);

    DL_ADC12_enableConversions(ADC12_0_INST);  // 现有
}

// ========== Track_Task（新增任务）==========
void Track_Task(void *pv) {
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // 阻塞等待 ISR 通知

        taskENTER_CRITICAL();
        // 拷贝 adc0[]、adc1[]、track → g_track_data（复用现有逻辑）
        taskEXIT_CRITICAL();

        // 计算循迹偏差等处理（从 UART_Task 中搬过来）
        g_track_data.deviation = calc_track_deviation(g_track_data.bits);

        xEventGroupSetBits(sensor_ready_evt, TRACK_READY_BIT);
    }
}

// ========== UART_TX_Task（重构现有 UART_Task）==========
void UART_TX_Task(void *pv) {
    for (;;) {
        // 等待 IMU 和 Track 数据都就绪，就绪后自动清除位
        xEventGroupWaitBits(sensor_ready_evt, ALL_SENSOR_BITS,
                            pdTRUE, pdFALSE, portMAX_DELAY);

        // 打包传感器帧（从 g_imu_raw 和 g_track_data）
        pack_sensor_frame(&tx_frame);
        uart_dma_send(&tx_frame);  // 使用中断/DMA 发送，不阻塞

        // 注意：不再需要 delay_ms(50)，靠 EventGroup 驱动
    }
}
```

### 2.6 app_main.c 扩展

```c
// 新增全局句柄（放在 head.h 或 app_main.h）
EventGroupHandle_t sensor_ready_evt;
TaskHandle_t Track_Task_Handle;

void app_main(void) {
    // --- 新增：创建 EventGroup ---
    sensor_ready_evt = xEventGroupCreate();
    configASSERT(sensor_ready_evt != NULL);

    // --- 现有任务创建（不变）---
    xTaskCreate(LED,  "LED",  configMINIMAL_STACK_SIZE,      NULL, tskIDLE_PRIORITY + 1, NULL);
    xTaskCreate(ICM,  "ICM",  configMINIMAL_STACK_SIZE * 10, NULL, tskIDLE_PRIORITY + 2, NULL);

    // --- 新增：Track_Task ---
    xTaskCreate(Track_Task, "Track", configMINIMAL_STACK_SIZE * 3, NULL,
                tskIDLE_PRIORITY + 2, &Track_Task_Handle);

    // --- UART_Task 改为 UART_TX（名称调整，保持现有栈大小）---
    xTaskCreate(UART_TX_Task, "UART_TX", configMINIMAL_STACK_SIZE * 5, NULL,
                tskIDLE_PRIORITY + 2, NULL);

    vTaskStartScheduler();
    while (1) {}
}
```

### 2.7 优先级说明

```
Prio 3: (预留，将来可能用于更高优先级任务)
Prio 2: ICM_Task, Track_Task, UART_TX_Task  ← 传感器采集和发送同级
Prio 1: LED_Task
Prio 0: Idle
```

ICM、Track、UART_TX 同优先级（Prio 2），配合 `configUSE_TIME_SLICING = 1` 时间片轮转：
- ICM 读完 IMU 后 `delay_ms(50)` 阻塞，让出 CPU
- Track 被通知唤醒后快速处理完设位，然后 `ulTaskNotifyTake` 阻塞
- UART_TX 等 EventGroup 两个位都就绪后发送，然后 `xEventGroupWaitBits` 阻塞

三者天然交替，不会互相饥饿。

---

## 三、car 端设计（串口接收 + PID 控制 + 安全保护）

### 3.1 现有基础（不动）

| 文件 | 内容 | 状态 |
|------|------|------|
| `main-blinky.c` / `main.c` | 入口函数 | 修复 `main_blinky()` 缺失问题（见下文） |
| `Driver/motor.c/h` | 电机驱动 | 不动 |
| `Driver/pid.c/h` | PID 控制器 | PID 计算从 ISR 移入任务，API 不动 |
| `Driver/uart.c/h` | UART 驱动 | 新增中断接收，保留发送函数兼容 |
| `Driver/delay.c/h` | 延时封装 | 不动 |

### 3.2 任务列表

| 任务 | 优先级 | 建议栈 | 周期 | 职责 |
|------|:---:|------|------|------|
| **UART_RX_Task**（新增） | 4 | 512B | 事件驱动 | 被 UART ISR 通知唤醒 → 解析帧 → Mutex 更新 `g_latest_sensor` |
| **PID_Task**（新增） | 3 | 512B | 10ms | `vTaskDelayUntil` 严格周期 → 取编码器 + 传感器 → PID 计算 → `motor_set()` |
| **Heartbeat_Task**（新增） | 1 | 256B | 1s | 监控传感器通信是否超时，超时则紧急停车 |

### 3.3 同步机制

| 机制 | 数量 | 发送方 | 接收方 | 用途 |
|------|:---:|--------|--------|------|
| **Task Notification** | 1 | UART_ISR | UART_RX_Task | ISR 收完一帧后唤醒解析 |
| **Mutex** | 1 | UART_RX_Task（写）/ PID_Task（读） | — | 保护共享 `g_latest_sensor`（持锁时间极短，仅结构体拷贝） |
| **Queue**（深度=1） | 1 | Timer_ISR | PID_Task | `xQueueOverwriteFromISR` 只保留最新编码器值，旧值覆盖 |
| **EventGroup** | 1 | UART_RX_Task / Heartbeat_Task | — | `SENSOR_UPDATED_BIT` 心跳监控位 |

### 3.4 任务/ISR 伪代码

```c
// ========== UART_ISR（uart.c 新增中断接收）==========
void UART_0_INST_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    uint8_t byte = DL_UART_receiveDataBlocking(UART_0_INST);

    // 喂入环形缓冲 + 帧解析状态机
    if (rx_frame_feed(&rx_ring, byte)) {   // 返回 true 表示完整帧就绪
        vTaskNotifyGiveFromISR(UART_RX_Task_Handle, &xHigherPriorityTaskWoken);
    }
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// ========== UART_RX_Task（新增）==========
void UART_RX_Task(void *pv) {
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);  // 等待 ISR 通知

        SensorFrame frame;
        if (parse_frame(&rx_ring, &frame) != OK) continue;  // CRC 不合格则丢弃

        xSemaphoreTake(sensor_mutex, portMAX_DELAY);
        g_latest_sensor = frame;   // 结构体赋值，很快
        xSemaphoreGive(sensor_mutex);

        xEventGroupSetBits(system_state, SENSOR_UPDATED_BIT);
    }
}

// ========== Timer_ISR（编码器采集，在 ISR 中只读数不计算）==========
void ENCODER_TIMER_IRQHandler(void) {
    BaseType_t xHigherPriorityTaskWoken = pdFALSE;
    int16_t left_cnt  = read_left_encoder();   // 读取硬件编码器寄存器
    int16_t right_cnt = read_right_encoder();
    EncoderData enc = { .left = left_cnt, .right = right_cnt };

    xQueueOverwriteFromISR(encoder_queue, &enc, &xHigherPriorityTaskWoken);  // 深度=1，只保留最新
    DL_Timer_clearInterruptStatus(ENCODER_TIMER_INST, DL_TIMERA_INTERRUPT_ZERO_EVENT);
    portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
}

// ========== PID_Task（新增，10ms 周期）==========
void PID_Task(void *pv) {
    TickType_t last_wake = xTaskGetTickCount();
    for (;;) {
        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(10));  // 严格 10ms

        EncoderData enc;
        SensorData  sensor;

        // 获取最新编码器（非阻塞，无新数据则跳过本轮）
        if (xQueueReceive(encoder_queue, &enc, 0) != pdTRUE) {
            motor_stop();  // 无编码器数据 = 异常，停车
            continue;
        }

        // 获取最新传感器数据（极短持锁）
        xSemaphoreTake(sensor_mutex, pdMS_TO_TICKS(5));
        sensor = g_latest_sensor;   // 快照拷贝
        xSemaphoreGive(sensor_mutex);

        // PID 计算（浮点运算在任务中，安全）
        float speed_left  = speed_cal(enc.left);
        float speed_right = speed_cal(enc.right);
        float pwm_left  = pid_cal(&pid_left,  speed_left);
        float pwm_right = pid_cal(&pid_right, speed_right);

        motor_left_set(pwm_left,  pid_left.direction);
        motor_right_set(pwm_right, pid_right.direction);
    }
}

// ========== Heartbeat_Task（新增，安全保护）==========
void Heartbeat_Task(void *pv) {
    for (;;) {
        EventBits_t bits = xEventGroupWaitBits(system_state, SENSOR_UPDATED_BIT,
                                                pdTRUE, pdFALSE, pdMS_TO_TICKS(1000));
        if ((bits & SENSOR_UPDATED_BIT) == 0) {
            // 1 秒内未收到传感器数据 → 紧急停车
            motor_stop();
            xEventGroupSetBits(system_state, SENSOR_TIMEOUT_BIT);
            // 持续等待直到恢复
            xEventGroupWaitBits(system_state, SENSOR_UPDATED_BIT,
                                pdFALSE, pdFALSE, portMAX_DELAY);
        }
    }
}
```

### 3.5 main.c 扩展

```c
#include "FreeRTOS.h"
#include "task.h"
#include "event_groups.h"
#include "queue.h"
#include "semphr.h"

// === 新增全局句柄 ===
SemaphoreHandle_t  sensor_mutex;
QueueHandle_t      encoder_queue;
EventGroupHandle_t system_state;
TaskHandle_t       UART_RX_Task_Handle;

#define SENSOR_UPDATED_BIT  (1 << 0)
#define SENSOR_TIMEOUT_BIT  (1 << 1)

int main(void) {
    prvSetupHardware();

    // 创建同步对象
    sensor_mutex  = xSemaphoreCreateMutex();
    encoder_queue = xQueueCreate(1, sizeof(EncoderData));  // 深度=1
    system_state  = xEventGroupCreate();

    // 创建任务
    xTaskCreate(UART_RX_Task,  "UART_RX",  256, NULL, tskIDLE_PRIORITY + 4, &UART_RX_Task_Handle);
    xTaskCreate(PID_Task,      "PID",      256, NULL, tskIDLE_PRIORITY + 3, NULL);
    xTaskCreate(Heartbeat_Task,"Heartbeat",128, NULL, tskIDLE_PRIORITY + 1, NULL);

    vTaskStartScheduler();
    return 0;
}
```

### 3.6 优先级说明

```
Prio 4: UART_RX_Task      ← 最高：串口数据不能丢，必须及时接收解析
Prio 3: PID_Task           ← 次高：10ms 控制周期，实时性要求高
Prio 2: (预留，传感器融合、决策层)
Prio 1: Heartbeat_Task     ← 最低：1s 检查一次，安全兜底
Prio 0: Idle
```

> **为什么 UART_RX 优先级高于 PID？**  
> UART_RX 持锁时间极短（只做一次结构体赋值），不会阻塞 PID_Task。  
> 高优先级确保串口不丢帧——帧丢了 PID 算得再快也没用。

---

## 四、串口通信协议

### 4.1 帧格式

```
│ 0xAA │ 0x55 │ Type (1B) │ Len (1B) │ Payload (N bytes) │ CRC8 (1B) │
```

- 帧头: `0xAA 0x55`
- Type: 帧类型
- Len: Payload 字节数（不含 CRC）
- Payload: 有效数据
- CRC8: 多项式 `0x07`（或 `0x31`），覆盖 Type + Len + Payload

### 4.2 数据类型

| Type | 含义 | Payload | 字节数 |
|:---:|------|---------|:---:|
| `0x01` | 完整传感器帧 | `track_dev`(float,4B) + `roll`(float,4B) + `pitch`(float,4B) + `yaw`(float,4B) | 16 |
| `0x02` | 仅 IMU | `roll`(float,4B) + `pitch`(float,4B) + `yaw`(float,4B) | 12 |
| `0x03` | 仅循迹 | `track_dev`(float,4B) + `track_bits`(uint8,1B) | 5 |
| `0xFF` | 心跳/应答 | 无 | 0 |

### 4.3 帧接收状态机

```
                          ┌── 0xAA ──────────────────────────────┐
                          ▼                                       │
┌──────┐  0xAA  ┌───────┐  0x55  ┌───────┐ 读Type ┌───────┐ 读Len ┌────────┐ 读N字节 ┌───────┐ CRC OK
│ IDLE │───────→│ HEAD1 │───────→│ HEAD2 │───────→│ TYPE  │──────→│ LENGTH │────────→│ DATA  │───────→│ CHECK │──────→ 回调 UART_RX_Task
└──────┘        └───────┘ 非0x55 └───────┘        └───────┘       └────────┘         └───────┘ CRC错  └───────┘
                     │         ┌────────────┐                                      回到IDLE
                     └────────→│ 回到 HEAD1 │  ← 如果收到的字节恰好又是 0xAA，可能是帧头
                               └────────────┘
```

---

## 五、FreeRTOS 配置补充建议

基于现有 `FreeRTOSConfig.h`，建议调整以下项目（不涉及代码改动，仅建议值）：

```c
// === 当前值 → 建议值 ===

#define INCLUDE_uxTaskGetStackHighWaterMark    0  → 1   // 启用栈水位检测，验证任务栈安全
#define configUSE_TIME_SLICING                 0  → 1   // 启用时间片轮转（blink_led 端同优先级任务需要）
#define configQUEUE_REGISTRY_SIZE              0  → 4   // 注册队列便于调试

// === 以下已正确配置，保持不变 ===
// configUSE_TASK_NOTIFICATIONS  1    ✅
// configUSE_EVENT_GROUPS        1    ✅
// configUSE_MUTEXES             1    ✅
// configUSE_PREEMPTION          1    ✅
// configCHECK_FOR_STACK_OVERFLOW 2   ✅
// configTICK_RATE_HZ            1000 ✅
// configMAX_PRIORITIES          10   ✅
```

---

## 六、数据流时序图

```
时间 →

blink_led 端:
  ADC_ISR    ──通知──► Track_Task ──设EventGroup位──┐
                                                     ├──► UART_TX_Task(等全部就绪) ──打包帧──► UART TX
  ICM_Task(50ms) ────────设EventGroup位──────────────┘

                                                                                    │ 串口
                                                                                    ▼
car 端:
  UART RX ──► UART_ISR ──通知──► UART_RX_Task ──Mutex写──► g_latest_sensor
                                                                  │
  Timer_ISR(编码器) ──Queue──► PID_Task(10ms) ──Mutex读────────────┘
                                                                  │
                                                              PID计算 → motor_set()

  Heartbeat_Task(1s) ──检查SENSOR_UPDATED_BIT──► 超时 → motor_stop()
```

---

## 七、安全设计

| 场景 | 保护机制 |
|------|---------|
| **串口断连 / blink_led 死机** | Heartbeat_Task 1 秒无数据 → 强制停车 |
| **编码器信号丢失** | PID_Task 收不到 `encoder_queue` 数据 → 停车 |
| **UART 帧 CRC 错误** | 解析失败直接丢弃，不更新 `g_latest_sensor` |
| **ICM 初始化永久失败** | ICM_Task 添加重试计数，超 N 次后跳过 IMU 数据（仍可仅靠循迹运行） |
| **PID 计算异常（NaN/Inf）** | 浮点运算后检查 `isfinite()`，异常则停车 |
| **任务栈溢出** | `INCLUDE_uxTaskGetStackHighWaterMark = 1` + `configCHECK_FOR_STACK_OVERFLOW = 2` |

---

## 八、选型理由

| 选择 | 为什么不选替代方案 |
|------|-------------------|
| **Task Notification** 替代二值信号量唤醒 | 零 RAM 开销（信号量需额外 `SemaphoreHandle_t`），Cortex-M0+ FreeRTOS 端口原生支持 |
| **EventGroup** 同步多传感器就绪 | 一次等待多个事件；多个二值信号量也能实现但代码更啰嗦 |
| **Mutex** 保护 `g_latest_sensor` | 持锁时间极短（仅结构体拷贝 < 10μs），不会阻塞；任务通知无法保护共享数据 |
| **Queue 深度=1** 传编码器 | `xQueueOverwriteFromISR` 自动覆盖旧值；PID 只关心最新值不关心历史 |
| **Heartbeat_Task 独立任务** | 与 PID_Task 解耦，即使 PID_Task 死循环也能被 watchdog 检测；逻辑更清晰 |

---

## 九、避开的坑

1. ~~Cortex-M0+ 无 BASEPRI，`taskENTER_CRITICAL()` 关所有中断~~ → 临界区代码保持极短，不做浮点或 I2C 操作
2. ~~ISR 中浮点运算~~ → 已全部移至任务
3. ~~ISR 中阻塞 UART 发送~~ → 改为 DMA/中断发送
4. ~~UART 任务 50ms 盲等轮询~~ → 改为 EventGroup 事件驱动
5. ~~`LineScan()` 函数不完整~~ → 循迹逻辑在 Track_Task 中重写
6. ~~`pthread.h` 误包含~~ → head.h 中删除

---

## 十、文件变更清单（仅计划，不做实际改动）

### blink_led 项目

| 文件 | 操作 | 内容 |
|------|:---:|------|
| `source/app_main.c` | 修改 | 新增 `sensor_ready_evt` 创建、Track_Task 创建、UART_Task 改名 |
| `source/2task.c` | 修改 | ICM_Task 加 `xEventGroupSetBits`；新增 `Track_Task` 函数；原 UART_Task 重构为 `UART_TX_Task` |
| `source/2task.h` | 修改 | 新增函数声明 |
| `Driver/track.c` | 修改 | ADC ISR 末尾加 `vTaskNotifyGiveFromISR` |
| `source/head.h` | 修改 | 新增 EventGroup/通知相关全局变量声明 |
| `source/uart.c` | 新增函数 | `uart_dma_send()`（不删原有函数） |

### car 项目

| 文件 | 操作 | 内容 |
|------|:---:|------|
| `main.c` | 修改 | 修复函数调用；新增同步对象和任务创建 |
| `main-blinky.c` | 删除/合并 | 合并到 `main.c`，消除 `main_blinky` vs `app_main` 混乱 |
| `Driver/pid.c` | 修改 | 从 Timer ISR 中移出 PID 计算逻辑，ISR 只读编码器入队列 |
| `Driver/uart.c` | 新增函数 | 中断接收 + 环形缓冲 + 帧解析状态机 |
| `source/protocol.c`（新增） | 新建 | 帧打包/解析 + CRC8 |
| `source/protocol.h`（新增） | 新建 | 协议结构体定义 |
| `source/tasks.c`（新增） | 新建 | `UART_RX_Task`、`PID_Task`、`Heartbeat_Task` 实现 |
| `Driver/head.h` | 修改 | 删除 `<pthread.h>` |

### 共享配置

| 文件 | 操作 | 内容 |
|------|:---:|------|
| `FreeRTOSConfig.h` | 修改 | 三项参数调整（见第五章） |

---

*设计书完*
