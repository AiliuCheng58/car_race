# MSPM0G3507 智能循迹小车项目说明

## 1. 项目概览

本项目是基于 TI MSPM0G3507 的 FreeRTOS 智能循迹小车工程。仓库根目录下包含当前主工程、FreeRTOS 依赖工程和早期参考工程，其中真正持续开发的是 `car/`、`blink_led_LP_MSPM0G3507_freertos_ticlang/`。

当前主控制链路已经不是早期“单纯 UART 循迹 + 双轮 PID”的版本，而是加入了按键门控、分段状态机、里程计和 DMP 姿态参与控制的版本。整体流程如下：

1. `main.c` 完成 SysConfig 外设初始化，然后依次初始化电机、按键、UART、OLED、DMP、编码器、里程计和速度环。
2. `app_main.c` 创建 `Line_Follow`、`OLED`、`KEY` 三个 FreeRTOS 任务。
3. 上电默认 `BUTTON_TASK_IDLE`，按键切到 `BUTTON_TASK_LINE_FOLLOW` 后才真正开始比赛逻辑。
4. `Line_Follow` 每 5 ms 运行一次，按 `SEG_A2C -> SEG_FOLLOW -> SEG_B2D` 状态机切换。
5. 斜线段使用 `odom.theta` 与目标角做角度闭环，跟随段使用 8 路循迹数据做差速闭环。
6. `TIMER_0_INST_IRQHandler()` 每 10 ms 读取编码器累计计数，执行左右轮速度 PID，并输出到 TB6612。
7. `OLED` 任务周期显示 RPM、PWM、编码器增量和循迹状态。

## 2. 工程组成

| 目录 | 作用 |
| --- | --- |
| `car/` | 当前主工程，比赛逻辑和驱动都在这里 |
| `freertos_builds_LP_MSPM0G3507_release_ticlang/` | `car` 依赖的 FreeRTOS 工程 |
| `blink_led_LP_MSPM0G3507_freertos_ticlang/` | 早期/参考示例工程，不是当前业务入口 |

## 3. 硬件与开发环境

| 项目 | 当前情况 |
| --- | --- |
| 主控芯片 | MSPM0G3507 |
| 工程类型 | CCS Theia / TI ARM Clang / FreeRTOS |
| 主工程 | `car` |
| 依赖工程 | `freertos_builds_LP_MSPM0G3507_release_ticlang` |
| 电机驱动 | TB6612 |
| 编码器 | 双轮 AB 相增量编码器 |
| 循迹输入 | UART 帧 `0xFE raw 0xEF` |
| 姿态输入 | MPU6050 DMP |
| 显示 | OLED |
| 调试配置 | `car/targetConfigs/MSPM0G3507.ccxml` 当前为 SEGGER J-Link |

## 4. 模块划分与功能

| 模块 | 主要文件 | 功能说明 |
| --- | --- | --- |
| 启动与总初始化 | `car/main.c` | 初始化外设、驱动、DMP、编码器、里程计、速度环，并启动 FreeRTOS |
| FreeRTOS 入口 | `car/app_main.c` | 创建 `Line_Follow`、`OLED`、`KEY` 三个任务 |
| 比赛任务层 | `car/2task.c` | 实现循迹状态机、OLED 遥测刷新、按键消抖和模式切换 |
| 循迹状态封装 | `car/line.c` | 维护循迹快照、超时、上升沿、下降沿和 OLED 可读快照 |
| 里程计与姿态 | `car/odom/odom.c` | 读取编码器累计增量和 DMP yaw，维护距离、朝向和目标角 |
| 控制闭环 | `car/Driver/control/control.c` | 私有持有左右轮 PID，在 10 ms 中断中完成速度闭环 |
| 编码器 | `car/Driver/encode/*` | 编码器累计计数、RPM 换算，对外提供 `encoder_get_total()` |
| PID | `car/Driver/pid/*` | PID 算法、目标更新、串口调参解析 |
| 电机驱动 | `car/Driver/motor/*` | TB6612 方向控制和 PWM 输出 |
| 循迹解析 | `car/Driver/track/*` | 从 UART 接收缓存中解析 `0xFE raw 0xEF` 循迹帧 |
| UART | `car/Driver/uart/*` | UART 接收中断、缓存维护和发送接口 |
| 按键 | `car/Driver/button/*` | 按键信号量、按下判定和模式切换 |
| OLED | `car/OLED/*` | OLED 显示驱动和字模 |
| MPU6050 | `car/mpu6050/*` | DMP 初始化、姿态读取和底层 I2C 适配 |

## 5. 集成功能说明

### 5.1 按键门控

系统上电后默认处于 `BUTTON_TASK_IDLE`。只有在 `KEY` 任务收到按键信号并切到 `BUTTON_TASK_LINE_FOLLOW` 后，`Line_Follow` 才会真正进入比赛逻辑。

未进入循迹模式时：

- `Line_Follow` 会持续 `control_stop()`
- 状态机会回到 `SEG_A2C`
- 边沿锁存、积分项、角度环历史值都会清零
- 循迹快照会复位

### 5.2 状态机循迹

`Line_Follow` 当前按三段状态运行：

| 状态 | 控制方式 | 切换依据 |
| --- | --- | --- |
| `SEG_A2C` | 基于目标角的角度闭环 | 到达白区长度并锁存到上升沿 |
| `SEG_FOLLOW` | 基于 8 路循迹误差的差速闭环 | 检测到下降沿且达到最小有效距离 |
| `SEG_B2D` | 基于目标角的角度闭环 | 逻辑与 `SEG_A2C` 对称 |

其中：

- `Line_track_rise()` 用于检测白区后重新压线
- `Line_track_fall()` 用于检测从黑线离开到白区
- `follow_exit_count` 用于决定 FOLLOW 离场后下一段进入 `SEG_A2C` 还是 `SEG_B2D`
- `odom_recalibrate_targets()` 会在 FOLLOW 离场时用当前 yaw 重算目标角

### 5.3 循迹输入与误差计算

循迹模块输入帧格式为：

```text
0xFE raw 0xEF
```

其中 `raw` 为 8 路循迹位图，`track.c` 中按左负右正权重计算偏差。当前逻辑特点如下：

- `Track_receive()` 从 UART 缓存中滑动找帧，不依赖固定对齐
- `Track_get_info()` 在 `raw == 0` 时可以沿用上一次有效偏差
- `line.c` 在此基础上补充了超时、边沿检测和遥测快照
- `LINE_FOLLOW_TIMEOUT_MS` 超时后按丢线处理

### 5.4 角度环与里程计

`odom` 模块同时维护：

- 当前朝向 `odom.theta`
- 当前阶段累计路程 `odom.distance`
- 本周期位移 `odom.delta_distance`
- 斜线段目标角 `odom.target_A2C`
- 斜线段目标角 `odom.target_B2D`

当前实现里：

- 里程位移来自编码器累计计数差分
- 朝向来自 DMP yaw
- `odom_wrap180()` 用于把角度统一约束到 `[-180, 180]`
- `odom_update()` 对 DMP 做隔次读取，降低 5 ms 循环中的阻塞频率

### 5.5 双轮速度闭环

`control.c` 在 `TIMER_0_INST_IRQHandler()` 中完成左右轮速度闭环：

1. 调用 `encoder_get_total()` 读取编码器累计计数
2. 与 `control_last_encoder_total` 做差，得到当前 10 ms 周期增量
3. 换算左右轮当前 RPM
4. 调用 `pid_cal()` 计算左右轮 PWM
5. 调用 `motor_left_set()`、`motor_right_set()` 输出到 TB6612
6. 保存 `ControlTelemetry` 遥测快照供 OLED 读取

这里的边界是当前工程比较重要的一点：

- `encode` 只提供累计真值，不做“取一次就清零”
- `control` 和 `odom` 各自保存自己的上一拍快照
- 左右轮 PID 由 `control.c` 私有持有，任务层不直接 `extern PID`

### 5.6 OLED 遥测

`OLED` 任务当前主要显示：

- 左右轮 RPM
- 左右轮 PWM
- 左右轮编码器增量
- 循迹 raw 位图
- 循迹有效标志和误差
- 系统运行时间

## 6. 关键参数

### 6.1 任务与控制周期

| 参数 | 当前值 |
| --- | --- |
| `LINE_FOLLOW_PERIOD_MS` | 5 ms |
| `LINE_FOLLOW_TIMEOUT_MS` | 500 ms |
| `PID_CONTROL_PERIOD_S` | 0.01 s |
| OLED 刷新周期 | 200 ms |

### 6.2 循迹参数

| 参数 | 当前值 |
| --- | --- |
| `LINE_FOLLOW_BASE_RPM` | 100.0f |
| `LINE_FOLLOW_KP` | 10.0f |
| `LINE_FOLLOW_KI` | 0.0f |
| `LINE_FOLLOW_KD` | 2.0f |
| `LINE_FOLLOW_TURN_RPM` | 100.0f |
| `LINE_FOLLOW_I_MAX` | 50.0f |

### 6.3 角度环参数

| 参数 | 当前值 |
| --- | --- |
| `ANGLE_KP` | 5.0f |
| `ANGLE_KI` | 0.0f |
| `ANGLE_KD` | 1.0f |
| `ANGLE_I_MAX` | 50.0f |
| `ROAD_WHITE` | 1.281f |
| `LINE_FOLLOW_MIN_DIST` | 0.3f |

### 6.4 编码器、电机与 PID 参数

| 参数 | 当前值 |
| --- | --- |
| `ENCODER_LEFT_DIR` | -1 |
| `ENCODER_RIGHT_DIR` | 1 |
| `ENCODER_LEFT_COUNTS_PER_REV` | 1458.6f |
| `ENCODER_RIGHT_COUNTS_PER_REV` | 1457.8f |
| `PID_DEFAULT_KP` | 2.0f |
| `PID_DEFAULT_KI` | 0.0f |
| `PID_DEFAULT_KD` | 0.0f |
| `PID_MIN_OUTPUT_PWM` | 120.0f |
| `MOTOR_PWM_MAX` | 1000 |

## 7. 项目结构

```text
aa/
├─ car/                                      # 当前智能小车主工程
│  ├─ main.c                                # 硬件初始化、启动任务调度
│  ├─ app_main.c / app_main.h               # FreeRTOS 任务创建
│  ├─ 2task.c / 2task.h                     # 比赛主逻辑、OLED、按键任务
│  ├─ line.c / line.h                       # 循迹快照、超时、边沿检测
│  ├─ car.syscfg                            # SysConfig 外设配置源
│  ├─ Driver/
│  │  ├─ button/                            # 按键驱动
│  │  ├─ control/                           # 10 ms 速度闭环
│  │  ├─ delay/                             # 延时
│  │  ├─ encode/                            # 编码器
│  │  ├─ motor/                             # 电机驱动
│  │  ├─ pid/                               # PID
│  │  ├─ track/                             # 循迹帧解析
│  │  └─ uart/                              # UART 收发
│  ├─ odom/                                 # 里程计与目标角
│  ├─ OLED/                                 # OLED 显示
│  ├─ mpu6050/                              # DMP 与 MPU6050 接口
│  ├─ freertos/                             # 启动文件与 RTOS 配置
│  └─ targetConfigs/                        # 调试器配置
├─ freertos_builds_LP_MSPM0G3507_release_ticlang/
│                                           # `car` 依赖的 FreeRTOS 工程
└─ blink_led_LP_MSPM0G3507_freertos_ticlang/
                                            # 寻迹板工程
```

## 8. 构建与使用

### 8.1 推荐方式

建议直接在 CCS Theia 中打开根目录工作区，并同时导入：

1. `car`
2. `freertos_builds_LP_MSPM0G3507_release_ticlang`

其中 `car` 是业务工程，`freertos_builds...` 是依赖工程。

### 8.2 修改入口

日常修改优先看这些文件：

1. `car/2task.c`
2. `car/line.c`
3. `car/odom/odom.c`
4. `car/Driver/control/control.c`
5. `car/Driver/track/track.c`
6. `car/car.syscfg`

说明：

- 比赛逻辑和切段主要在 `2task.c`
- 循迹状态封装在 `line.c`
- 里程和姿态处理在 `odom.c`
- 速度闭环在 `control.c`
- 循迹串口帧解析在 `track.c`
- 外设、引脚和定时器修改回到 `car.syscfg`

## 9. 维护注意点

- 外设、引脚、时钟等配置优先回到 `car/car.syscfg` 修改。
- 当前模块边界已经整理过：`control` 持有 PID，`encode` 维护累计值，`odom` 和 `control` 各自做差。
- 如果要继续扩比赛状态机，优先沿用 `Line_Follow` 当前的分段、边沿锁存和模式门控思路。

## 10. 当前状态小结

当前主线已经接入：

- 按键启动门控
- 8 路循迹解析
- 分段状态机循迹
- 编码器测速
- 10 ms 双轮速度 PID
- DMP 姿态参与转向
- OLED 遥测显示
