# MSPM0G3507 智能循迹小车项目说明

## 1. 项目概览

本项目是基于 TI MSPM0G3507 的 FreeRTOS 智能循迹小车工程。工程主体位于 `car/`，由 SysConfig 生成底层外设初始化，应用层通过 FreeRTOS 任务、中断、PID 控制和电机驱动完成自动循迹。

核心功能链路如下：

1. UART0 接收 8 路循迹模块数据帧，帧格式为 `0xFE raw 0xEF`。
2. `Line_Follow` 任务每 5 ms 解析循迹数据，计算车身相对黑线的偏差。
3. 循迹偏差经 P/D 差速算法转换成左右轮目标 RPM。
4. TIMA1 每 10 ms 进入定时器中断，读取编码器计数并执行左右轮速度 PID。
5. PID 输出写入 TIMA0 PWM 通道，再通过 TB6612 电机驱动控制左右轮。
6. `OLED` 任务每 200 ms 显示转速、PWM、编码器计数和循迹状态。
7. 按键通过 GPIOB 中断唤醒 `KEY` 任务，可切换 `button_task` 模式变量。

## 2. 硬件与开发环境

| 项目 | 当前配置 |
| --- | --- |
| 主控芯片 | MSPM0G3507 |
| 封装 | LQFP-64(PM) |
| 工程类型 | CCS Theia / TI ARM Clang / FreeRTOS |
| SDK | mspm0_sdk 2.10.00.04 |
| SysConfig | `car.syscfg`，工具版本元数据为 `1.27.1+4634` |
| 编译器 | TICLANG_4.0.0.LTS |
| CPU 主频 | 32 MHz |
| PWM 定时器 | TIMA0，8 MHz 计数时钟，周期 1000，约 8 kHz PWM |
| PID 定时器 | TIMA1，10 ms 周期中断 |
| 调试目标 | `targetConfigs/MSPM0G3507.ccxml` 当前为 SEGGER J-Link |
| FreeRTOS 依赖 | 工作区工程 `freertos_builds_LP_MSPM0G3507_release_ticlang` |

注意：`.ccsproject` 中仍能看到早期 TIXDS110 连接配置，但当前 active target config 文件使用的是 SEGGER J-Link。烧录前请按实际调试器确认目标配置。

## 3. 模块划分与功能

| 模块 | 主要文件 | 功能说明 |
| --- | --- | --- |
| 启动与总初始化 | `main.c` | 调用 `SYSCFG_DL_init()`，初始化电机、按键、编码器、UART、OLED 和控制模块，启动 PWM、PID 定时器和 FreeRTOS。 |
| FreeRTOS 应用入口 | `app_main.c/.h` | 创建 `Line_Follow`、`OLED`、`KEY` 三个任务，提供静态内存分配钩子和栈溢出钩子。 |
| 任务层 | `2task.c/.h` | 实现循迹控制任务、OLED 遥测任务和按键任务。 |
| 电机驱动 | `Driver/motor.c/.h` | 封装 TB6612 方向脚和 PWM 输出，支持左右轮正反转、停车、PWM 限幅和方向系数校准。 |
| 编码器 | `Driver/encode.c/.h` | 使用左右轮 AB 相 GPIO 边沿中断解码增量，按 10 ms 采样周期换算 RPM。 |
| PID 控制 | `Driver/pid.c/.h` | 支持速度式/位置式 PID，包含输出限幅、积分限幅、最小起转 PWM 和串口调参解析函数。 |
| 控制闭环 | `Driver/control.c/.h` | 初始化左右轮速度 PID，在 TIMA1 中断中读取编码器、计算 PID、输出电机 PWM，并维护遥测快照。 |
| UART | `Driver/uart.c/.h` | UART0 接收中断、软件接收缓存、错误中断恢复、阻塞字符串发送。 |
| 循迹解析 | `Driver/track.c/.h` | 从 UART 缓存查找循迹帧，解析 8 路 raw 数据，计算左负右正的循迹偏差。 |
| 按键 | `Driver/button.c/.h` | 创建按键信号量，读取低电平按下状态，按键中断后在任务中消抖并切换模式变量。 |
| OLED 显示 | `oled/oled.c/.h`、`oled/oledfont.h` | 使用 PB9/PB8 软件模拟 I2C，驱动 SSD1306 OLED 显示遥测信息。 |
| MPU6050/DMP | `mpu6050/*` | 已移植 MPU6050 I2C 读写、DMP 初始化和姿态读取接口；当前主流程尚未调用。 |
| 阻塞延时 | `Driver/delay.c/.h` | 基于 `DL_Common_delayCycles()` 实现毫秒级阻塞延时，主要用于 OLED 初始化。 |
| SysConfig 外设配置 | `car.syscfg`、`Debug/ti_msp_dl_config.*` | 配置 GPIO、PWM、Timer、I2C、UART、时钟和中断。`Debug/ti_msp_dl_config.*` 为生成文件，只读不手改。 |

## 4. 集成功能说明

### 4.1 自动循迹

循迹模块通过 UART0 上报 3 字节帧：

```text
0xFE raw 0xEF
```

`raw` 为 8 路循迹状态，`bit0` 在最左，`bit7` 在最右，代码中约定 `1` 表示对应探头压到黑线。`Track_line()` 使用权重 `[-7, -5, -3, -1, 1, 3, 5, 7]` 计算偏差，偏差左负右正。

`Line_Follow` 任务每 5 ms 执行一次：

- 收到新帧则更新循迹状态。
- 500 ms 内没有新循迹帧，或当前没有可靠偏差，则左右轮目标速度置 0。
- 有效偏差下使用 `turn = error * LINE_FOLLOW_KP + delta_error * LINE_FOLLOW_KD` 计算差速。
- 差速经过 `TURN_RPM` 限幅和平滑后，设置左右轮目标转速：

```text
left_target  = BASE_RPM + turn
right_target = BASE_RPM - turn
```

当前默认参数：

| 参数 | 值 | 说明 |
| --- | --- | --- |
| `BASE_RPM` | 50.0 | 基础前进速度 |
| `LINE_FOLLOW_KP` | 7.0 | 循迹比例系数 |
| `LINE_FOLLOW_KD` | 2.0 | 循迹微分系数 |
| `TURN_RPM` | 35.0 | 转向修正限幅 |
| `LOOP_MS` | 5 ms | 循迹任务周期 |
| `TIMEOUT_MS` | 500 ms | 循迹帧超时停车时间 |

### 4.2 双轮速度闭环

`control.c` 在 `TIMER_0_INST_IRQHandler()` 中完成 10 ms 闭环：

1. 调用 `encoder_get_and_clear()` 获取左右轮本周期计数。
2. 按实测每圈计数换算 RPM。
3. 通过 `pid_cal()` 计算左右轮 PID 输出。
4. 调用 `motor_left_set()`、`motor_right_set()` 更新 TB6612 输出。
5. 保存遥测数据供 OLED 任务显示。

当前编码器参数：

| 参数 | 值 |
| --- | --- |
| 左轮方向系数 | `ENCODER_LEFT_DIR = -1` |
| 右轮方向系数 | `ENCODER_RIGHT_DIR = 1` |
| 左轮每圈计数 | `1458.6` |
| 右轮每圈计数 | `1457.8` |
| 采样周期 | `0.01 s` |

当前 PID 默认参数：

| 参数 | 值 | 说明 |
| --- | --- | --- |
| `PID_DEFAULT_KP` | 1.1 | 默认比例系数 |
| `PID_DEFAULT_KI` | 0.0 | 默认关闭积分 |
| `PID_DEFAULT_KD` | 0.0 | 默认关闭微分 |
| `PID_MIN_OUTPUT_PWM` | 120.0 | 非零目标下的最小起转 PWM |
| `MOTOR_PWM_MAX` | 1000 | PWM 满占空比比较值 |

`pid_read()` 已实现串口 PID 调参解析，命令格式示例：

```text
kp=1.0, ki=0.1, kd=0.0
```

当前主任务链路中没有周期调用 `pid_read()`。如需启用串口调参，需要在合适任务中调用，并注意它与循迹帧共用 UART 接收缓存。

### 4.3 OLED 遥测显示

OLED 使用 PB9/PB8 软件模拟 I2C，不占用硬件 I2C 外设。`OLED` 任务每 200 ms 显示：

- 左右轮 RPM
- 左右轮 PWM 输出
- 左右轮编码器计数
- 循迹 raw 原始值
- 8 路循迹 bit 状态
- 当前循迹偏差和有效标志
- 系统运行秒数

### 4.4 按键模式变量

KEY 接在 PB21，配置为上拉输入、下降沿中断。中断中释放二值信号量，`KEY` 任务完成 20 ms 消抖后调用 `Button_switch()`，使 `button_task` 在 `0..3` 间循环。

当前循迹控制逻辑暂未根据 `button_task` 分支执行，后续可以用它扩展停车、循迹、调参、姿态显示等模式。

### 4.5 MPU6050 姿态接口

SysConfig 已配置 I2C0 实例 `MPU6050`，SDA/SCL 为 PA0/PA1，速率 100 kHz。`mpu6050/` 目录中包含 InvenSense 驱动、DMP 固件接口和 `mpu_port.c` 的底层读写适配。

当前 `main.c` 没有调用 `DMP_Init()` 或 `DMP_Read_Data()`，因此 MPU6050 属于已集成代码和外设配置、尚未接入主控制流程的功能。`DMP_Init()` 内部会配置 SysTick，若要在 FreeRTOS 运行时启用，需要额外确认它和 FreeRTOS tick 的关系。

## 5. 引脚配置

以下引脚来自 `car.syscfg` 和 `Debug/ti_msp_dl_config.h`。其中 `Debug/ti_msp_dl_config.h` 是生成文件，只用于核对宏名和实际引脚。

### 5.1 电机与 PWM

| 功能 | SysConfig 名称 | MCU 引脚 | 外设/通道 | 方向/说明 |
| --- | --- | --- | --- | --- |
| 左轮方向 1 | `MOTOR_LEFT_LEFT_IN1` | PA24 | GPIOA | TB6612 左路 IN1 |
| 左轮方向 2 | `MOTOR_LEFT_LEFT_IN2` | PA25 | GPIOA | TB6612 左路 IN2 |
| 右轮方向 1 | `MOTOR_RIGHT_RIGHT_IN1` | PB25 | GPIOB | TB6612 右路 IN1 |
| 右轮方向 2 | `MOTOR_RIGHT_RIGHT_IN2` | PB26 | GPIOB | TB6612 右路 IN2 |
| 电机待机使能 | `MOTOR_ENABLE_STBY` | PA23 | GPIOA | 上电初始化置高，TB6612 脱离待机 |
| 左轮 PWM | `GPIO_PWM_0_C0` | PB14 | TIMA0 CCP0 | `GPIO_PWM_0_C0_IDX` |
| 右轮 PWM | `GPIO_PWM_0_C1` | PB12 | TIMA0 CCP1 | `GPIO_PWM_0_C1_IDX` |

### 5.2 编码器

| 功能 | SysConfig 名称 | MCU 引脚 | 中断 | 说明 |
| --- | --- | --- | --- | --- |
| 左轮 A 相 | `ENCODER_LEFT_LEFT_A` | PA8 | GPIOA group interrupt | 上拉输入，双边沿触发 |
| 左轮 B 相 | `ENCODER_LEFT_LEFT_B` | PA7 | GPIOA group interrupt | 上拉输入，双边沿触发 |
| 右轮 A 相 | `ENCODER_RIGHT_RIGHT_A` | PB7 | GPIOB group interrupt | 上拉输入，双边沿触发 |
| 右轮 B 相 | `ENCODER_RIGHT_RIGHT_B` | PB10 | GPIOB group interrupt | 上拉输入，双边沿触发 |

编码器和按键共用 `GROUP1_IRQHandler()` 处理。GPIOB 侧右轮编码器与按键同属 `GPIO_MULTIPLE_GPIOB_INT_IRQN`。

### 5.3 通信、显示与传感器

| 功能 | SysConfig 名称 | MCU 引脚 | 外设 | 参数/说明 |
| --- | --- | --- | --- | --- |
| UART 发送 | `GPIO_UART_0_TX` | PA10 | UART0 TX | 115200, 8N1 |
| UART 接收 | `GPIO_UART_0_RX` | PA11 | UART0 RX | RX FIFO 1 字节阈值，接收中断 |
| MPU6050 SDA | `GPIO_MPU6050_SDA` | PA0 | I2C0 SDA | 100 kHz，硬件 I2C |
| MPU6050 SCL | `GPIO_MPU6050_SCL` | PA1 | I2C0 SCL | 100 kHz，硬件 I2C |
| OLED SCL | `GPIO_OLED_PIN_SCL` | PB9 | GPIO 模拟 I2C | SSD1306 软件 I2C 时钟 |
| OLED SDA | `GPIO_OLED_PIN_SDA` | PB8 | GPIO 模拟 I2C | SSD1306 软件 I2C 数据 |

### 5.4 人机交互与调试

| 功能 | SysConfig 名称 | MCU 引脚 | 说明 |
| --- | --- | --- | --- |
| LED | `LED_PIN_22` | PB22 | GPIO 输出，初始化置高 |
| 按键 | `KEY_PIN_21` | PB21 | 上拉输入，低电平按下，下降沿中断 |
| SWCLK | Board debug | PA20 | SWD 调试时钟 |
| SWDIO | Board debug | PA19 | SWD 调试数据 |

## 6. 中断与实时任务

| 来源 | 函数 | 优先级/周期 | 作用 |
| --- | --- | --- | --- |
| TIMA1 | `TIMER_0_INST_IRQHandler()` | 中断优先级 0，10 ms | 速度 PID、编码器采样、电机 PWM 更新 |
| GPIOA/GPIOB group | `GROUP1_IRQHandler()` | GPIOB 优先级 1 | 编码器 AB 相解码、按键信号量释放 |
| UART0 | `UART_0_INST_IRQHandler()` | 中断优先级 1 | 接收循迹帧字节，处理 UART 接收错误 |
| FreeRTOS 任务 | `Line_Follow` | `tskIDLE_PRIORITY + 2`，5 ms | 循迹数据解析、目标速度更新 |
| FreeRTOS 任务 | `OLED` | `tskIDLE_PRIORITY + 1`，200 ms | 显示遥测信息 |
| FreeRTOS 任务 | `KEY` | `tskIDLE_PRIORITY + 1` | 按键消抖和模式变量切换 |

## 7. 项目结构

```text
aa/
├─ car/                                      # 当前智能小车主工程
│  ├─ main.c                                # 硬件初始化、定时器/PWM 启动、进入 app_main
│  ├─ app_main.c / app_main.h               # FreeRTOS 任务创建与系统钩子
│  ├─ 2task.c / 2task.h                     # 循迹、OLED、按键任务
│  ├─ car.syscfg                            # SysConfig 外设、引脚、时钟、中断配置源文件
│  ├─ mspm0g3507.cmd                        # 链接脚本
│  ├─ Driver/
│  │  ├─ motor.c / motor.h                  # TB6612 电机与 PWM 输出
│  │  ├─ encode.c / encode.h                # 编码器中断解码与 RPM 换算
│  │  ├─ pid.c / pid.h                      # PID 控制与串口参数解析
│  │  ├─ control.c / control.h              # 10 ms 速度闭环与遥测快照
│  │  ├─ uart.c / uart.h                    # UART0 接收缓存与发送
│  │  ├─ track.c / track.h                  # 循迹帧解析和偏差计算
│  │  ├─ button.c / button.h                # 按键中断、信号量、模式变量
│  │  └─ delay.c / delay.h                  # 阻塞延时
│  ├─ oled/
│  │  ├─ oled.c / oled.h                    # SSD1306 OLED 软件 I2C 驱动
│  │  ├─ oledfont.h                         # 字库
│  │  └─ bmp.h                              # 图片数据头文件
│  ├─ mpu6050/
│  │  ├─ mpu_port.c / mpu_port.h            # MSPM0 I2C 适配与 DMP 读取封装
│  │  ├─ inv_mpu.c / inv_mpu.h              # InvenSense MPU 驱动
│  │  └─ inv_mpu_dmp_motion_driver.*        # DMP 驱动
│  ├─ freertos/ticlang/
│  │  └─ startup_mspm0g350x_ticlang.c       # 启动文件和中断向量表
│  ├─ targetConfigs/
│  │  └─ MSPM0G3507.ccxml                   # 调试/烧录目标配置，当前为 J-Link
│  └─ Debug/                                # CCS 生成目录，含 ti_msp_dl_config.*、car.out 等
├─ freertos_builds_LP_MSPM0G3507_release_ticlang/
│                                           # FreeRTOS 库依赖工程
└─ blink_led_LP_MSPM0G3507_freertos_ticlang/
                                            # 早期/参考 FreeRTOS 示例工程
```

## 8. 构建与烧录

### 8.1 在 CCS Theia 中构建

1. 打开工作区 `aa/`。
2. 确认同时导入 `car` 和 `freertos_builds_LP_MSPM0G3507_release_ticlang`。
3. 选择 `car` 工程的 `Debug` 配置构建。
4. 构建成功后输出文件为 `car/Debug/car.out`。

### 8.2 命令行构建

当前工程已生成 `Debug` makefile，可在项目根目录执行：

```powershell
gmake -C "D:\AiliuCheng\work\DIAN_RACE\aa\car\Debug" clean all
```

如需要重新生成 SysConfig 输出，可参考检查脚本给出的命令：

```powershell
"D:\AiliuCheng\work\DIAN_RACE\TiSDK\sysconfig_cli.bat" --script "D:\AiliuCheng\work\DIAN_RACE\aa\car\car.syscfg" -o "." -s "D:\AiliuCheng\work\DIAN_RACE\TiSDK\mspm0_sdk_2_10_00_04\.metadata\product.json" --compiler ticlang
```

### 8.3 烧录

当前 `targetConfigs/MSPM0G3507.ccxml` 使用 SEGGER J-Link。确认硬件连接后，可参考：

```powershell
dslite -c "D:\AiliuCheng\work\DIAN_RACE\aa\car\targetConfigs\MSPM0G3507.ccxml" -e -r 2 -u "D:\AiliuCheng\work\DIAN_RACE\aa\car\Debug\car.out"
```

如果使用 XDS110，需要先在 CCS 中切换或重新生成对应的 target configuration。

## 9. 关键调试点

- 电机方向反了：优先修改 `MOTOR_LEFT_FORWARD_SIGN` 或 `MOTOR_RIGHT_FORWARD_SIGN`。
- 编码器前进计数方向反了：修改 `ENCODER_LEFT_DIR` 或 `ENCODER_RIGHT_DIR`。
- 车不动但 PID 有输出：检查 `PID_MIN_OUTPUT_PWM`、TB6612 STBY、方向脚和 PWM 引脚接线。
- 循迹无响应：确认 UART0 RX/TX 接线、波特率 115200、帧格式 `0xFE raw 0xEF`、`raw` 中黑线位是否为 1。
- OLED 不亮：确认 OLED 接 PB9/PB8，当前是软件 I2C，不是 PA0/PA1 的硬件 I2C。
- MPU6050 读不到：确认接 PA0/PA1 硬件 I2C、上拉电阻、电源电平和 I2C 地址。
- PID 中断不执行：确认 `TIMER_0_INST_INT_IRQN` 已使能，`DL_Timer_startCounter(TIMER_0_INST)` 已调用。
- 不要手动修改 `Debug/ti_msp_dl_config.c` 或 `Debug/ti_msp_dl_config.h`，外设和引脚应从 `car.syscfg` 修改后重新生成。

## 10. 当前状态小结

当前主流程已经接入：电机 PWM、编码器测速、10 ms 速度 PID、UART 循迹解析、5 ms 循迹任务、OLED 遥测显示、按键中断消抖。

当前已实现但尚未接入主控制流程：MPU6050/DMP 姿态读取、串口 PID 调参周期调用、`button_task` 对运行模式的实际分支控制。
