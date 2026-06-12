# DIAN_RACE

基于 TI MSPM0G3507 的 FreeRTOS 小车工程仓库。当前真正使用的主工程是 `car/`，其余目录分别用于 FreeRTOS 依赖工程和早期参考工程。

## 目录说明

```text
aa/
├─ car/                                         # 当前主工程
├─ freertos_builds_LP_MSPM0G3507_release_ticlang/
│                                               # FreeRTOS 依赖工程
├─ blink_led_LP_MSPM0G3507_freertos_ticlang/    # 早期/参考示例工程
├─ aa.theia-workspace                           # Theia 工作区文件
└─ README.md
```

## 当前主流程

`car/` 不是单纯的“串口循迹 + PID”老版本了，当前主流程已经是这条链路：

1. `main.c` 完成 SysConfig 外设初始化，然后依次初始化电机、按键、UART、OLED、DMP、编码器、里程计、速度环。
2. `app_main.c` 创建 3 个任务：`Line_Follow`、`OLED`、`KEY`。
3. `KEY` 任务通过 `button_task` 控制运行模式。上电默认 `BUTTON_TASK_IDLE`，第一次按键才进入 `BUTTON_TASK_LINE_FOLLOW`。
4. `Line_Follow` 每 5 ms 运行一次，按 `SEG_A2C -> SEG_FOLLOW -> SEG_B2D` 的状态机工作。
5. 斜线段使用 `odom.theta` 和目标角做角度闭环，跟随段使用 8 路循迹数据做差速闭环。
6. `TIMER_0_INST_IRQHandler()` 每 10 ms 读取编码器累计值，执行左右轮 PID，并输出到 TB6612。
7. `OLED` 任务周期显示 RPM、PWM、编码器增量和循迹状态。

## `car/` 工程结构

```text
car/
├─ main.c
├─ app_main.c / app_main.h
├─ 2task.c / 2task.h
├─ line.c / line.h
├─ car.syscfg
├─ targetConfigs/
├─ Driver/
│  ├─ button/      # 按键中断、消抖、模式切换
│  ├─ control/     # 10ms 速度闭环，PID 私有持有者
│  ├─ delay/       # 阻塞延时
│  ├─ encode/      # 编码器累计计数与 RPM 换算
│  ├─ motor/       # TB6612 PWM/方向输出
│  ├─ pid/         # PID 算法与串口调参解析
│  ├─ track/       # 8 路循迹数据解析
│  └─ uart/        # UART 接收缓存与发送
├─ odom/
│  ├─ odom.c
│  └─ odom.h
├─ OLED/
├─ mpu6050/
└─ freertos/
```

## 关键模块关系

### 1. 任务层

- `Line_Follow()` 是比赛主逻辑，既负责循迹，也负责分段切换。
- `KEY()` 只做按键同步和模式切换。
- `OLED()` 只读遥测快照，不直接参与控制。

### 2. 编码器与控制

- `encode` 模块只维护“累计计数”。
- `control` 和 `odom` 都通过 `encoder_get_total()` 取快照，再各自计算增量。
- 左右轮 PID 由 `control.c` 私有持有，任务层只通过 `control_set_target_rpm()`、`control_stop()` 访问。

### 3. 循迹与状态机

- `track` 模块负责从 UART 缓存里解析循迹帧。
- `line` 模块负责维护循迹快照、超时、上升沿和下降沿。
- `2task.c` 里的 `RaceSegment` 状态机根据循迹边沿和里程信息切段。

### 4. 姿态与里程

- `odom` 使用编码器累计位移和 DMP yaw。
- `odom_recalibrate_targets()` 会在 FOLLOW 离场进入弧线段时，用当前 yaw 重算目标角。
- 当前 `odom_update()` 里对 DMP 读数做了隔次读取，避免每个 5 ms 周期都阻塞在姿态读取上。

## 当前控制参数

### 任务与闭环周期

| 项目 | 当前值 |
| --- | --- |
| `LINE_FOLLOW_PERIOD_MS` | 5 ms |
| `LINE_FOLLOW_TIMEOUT_MS` | 500 ms |
| `PID_CONTROL_PERIOD_S` | 0.01 s |
| OLED 刷新周期 | 200 ms |

### 循迹参数

| 宏 | 当前值 |
| --- | --- |
| `LINE_FOLLOW_BASE_RPM` | 100.0f |
| `LINE_FOLLOW_KP` | 10.0f |
| `LINE_FOLLOW_KI` | 0.0f |
| `LINE_FOLLOW_KD` | 2.0f |
| `LINE_FOLLOW_TURN_RPM` | 100.0f |

### 角度环参数

| 宏 | 当前值 |
| --- | --- |
| `ANGLE_KP` | 5.0f |
| `ANGLE_KI` | 0.0f |
| `ANGLE_KD` | 1.0f |
| `ROAD_WHITE` | 1.281f |
| `LINE_FOLLOW_MIN_DIST` | 0.3f |

### 编码器与电机参数

| 宏 | 当前值 |
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

## 外设与接口要点

- 主控：MSPM0G3507
- 实时系统：FreeRTOS
- 电机驱动：TB6612
- 编码器：双轮 AB 相中断计数
- 循迹输入：UART 帧 `0xFE raw 0xEF`
- 姿态输入：MPU6050 DMP
- 显示：OLED
- 当前 `car/targetConfigs/MSPM0G3507.ccxml` 使用的是 SEGGER J-Link 配置

## 构建使用

建议直接在 CCS Theia 中打开根目录工作区，并同时导入：

- `car`
- `freertos_builds_LP_MSPM0G3507_release_ticlang`

其中：

- `car` 是业务工程
- `freertos_builds_LP_MSPM0G3507_release_ticlang` 是依赖工程
- `blink_led_LP_MSPM0G3507_freertos_ticlang` 只是参考，不是当前主线

仓库里存在 `Debug/` 等生成目录，但它们不是日常阅读入口；外设、引脚和时钟修改应回到 `car/car.syscfg` 和源码本体。

## 阅读建议

如果要快速理解当前比赛逻辑，推荐按这个顺序看：

1. `car/2task.c`
2. `car/line.c`
3. `car/odom/odom.c`
4. `car/Driver/control/control.c`
5. `car/Driver/track/track.c`
6. `car/Driver/encode/encode.c`

## 当前状态小结

当前主线已经接入：

- 按键启动门控
- 8 路循迹解析
- 分段状态机循迹
- 编码器测速
- 10 ms 双轮速度 PID
- DMP 姿态参与转向
- OLED 遥测显示

当前仓库里同时保留了依赖工程与参考工程，因此后续开发默认应以 `car/` 为中心，不要把 `blink_led...` 误当成当前业务入口。
