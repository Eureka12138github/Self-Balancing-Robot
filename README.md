# 平衡车项目 - PlatformIO 版本

[![PlatformIO Build](https://img.shields.io/badge/PlatformIO-Build-success)](https://platformio.org)
[![STM32F103C8](https://img.shields.io/badge/MCU-STM32F103C8T6-blue)](https://www.st.com/en/microcontrollers-microprocessors/stm32f103c8.html)
[![License: MIT](https://img.shields.io/badge/License-MIT-yellow.svg)](LICENSE)

## 📖 项目简介

本项目是基于 **STM32F103C8T6** 最小系统板的双轮平衡车控制系统，采用 **PlatformIO** 作为开发环境。

### 核心功能

- ✅ **自平衡控制**：基于 MPU6050 的姿态解算与 PID 控制
- ✅ **速度闭环**：霍尔编码器反馈的速度环控制
- ✅ **转向控制**：差速转向算法
- ✅ **蓝牙遥控**：通过手机 APP 实时控制
- ✅ **OLED 菜单**：可视化参数配置界面
- ✅ **看门狗保护**：独立看门狗防止系统死机

### 主要特性

| 项目 | 规格 |
|------|------|
| MCU | STM32F103C8T6 (ARM Cortex-M3, 72MHz) |
| IMU | MPU6050 (6 轴陀螺仪 + 加速度计) |
| 电机驱动 | TB6612FNG 双路 H 桥驱动 |
| 编码器 | 霍尔编码器（44PPR） |
| 显示屏 | 0.96" OLED (I2C, 128x64) |
| 通信 | HC-05/HC-06 蓝牙模块 |
| 电源 | 2S 锂电池 (7.4V) |

---

## 🚀 快速开始

### 前置要求

#### 硬件要求

- **开发板**: STM32F103C8T6 最小系统板（Blue Pill）
- **调试器**: CMSIS-DAP / ST-Link V2
- **传感器**: MPU6050 模块
- **电机**: N20 减速电机（带霍尔编码器）× 2
- **显示器**: 0.96" OLED I2C 显示屏
- **其他**: 杜邦线、面包板、电池等

#### 软件要求

- **操作系统**: Windows 10/11, macOS, Linux
- **IDE**: Visual Studio Code
- **工具链**: 
  - PlatformIO IDE 扩展
  - Git 版本控制

### 安装步骤

#### 1. 安装 VSCode 和 PlatformIO

```bash
# 1. 下载安装 VSCode
# https://code.visualstudio.com/download

# 2. 在 VSCode 中安装 PlatformIO 插件
# 扩展商店搜索 "PlatformIO IDE" 并安装

# 3. 等待 PlatformIO 自动下载工具链（首次启动较慢）
```

#### 2. 克隆项目

```bash
# 克隆仓库
git clone https://github.com/your-username/balanced-car-pio.git
cd balanced-car-pio

# 打开项目
code .
```

#### 3. 配置项目

```ini
; platformio.ini 已预配置
; 如需修改开发板型号或上传协议，请编辑此文件

[env:genericSTM32F103C8]
platform = ststm32
board = genericSTM32F103C8
framework = cmsis

upload_protocol = cmsis-dap  ; 可改为 stlink, jlink 等
upload_speed = 2000000
```

#### 4. 编译项目

```bash
# 使用 PlatformIO 工具栏点击 "Build" 按钮
# 或使用命令：
pio run
```

#### 5. 烧录程序

```bash
# 连接开发板和调试器后，点击 "Upload" 按钮
# 或使用命令：
pio run --target upload
```

---

## 🔧 硬件连接

### 引脚分配图

```
STM32F103C8T6
┌─────────────────────────┐
│    ┌─────────────┐      │
│    │   STM32     │      │
│    │  F103C8T6   │      │
│    └─────────────┘      │
│                         │
│  PB6 ──── E1A (左编码器 A)│
│  PB7 ──── E1B (左编码器 B)│
│  PA6 ──── E2A (右编码器 A)│
│  PA7 ──── E2B (右编码器 B)│
│                         │
│  PA0 ──── PWMA (电机 A PWM)│
│  PB13 ─── AIN1          │
│  PB12 ─── AIN2          │
│                         │
│  PA1 ──── PWMB (电机 B PWM)│
│  PB15 ─── BIN1          │
│  PB14 ─── BIN2          │
│                         │
│  PB10 ── SCL (MPU6050)  │
│  PB11 ── SDA (MPU6050)  │
│                         │
│  PB8 ─── SCL (OLED)     │
│  PB9 ─── SDA (OLED)     │
│                         │
│  PA2 ─── TX (蓝牙)      │
│  PA3 ─── RX (蓝牙)      │
│                         │
│  PB0 ─── KEY1           │
│  PA11 ── KEY2           │
└─────────────────────────┘
```

### 接线注意事项

⚠️ **重要提示：**

1. **电平匹配**: MPU6050 和 OLED 使用 3.3V 供电，确保不要接到 5V
2. **共地**: 所有模块的 GND 必须共地
3. **电机干扰**: 电机动力线尽量短，并远离信号线
4. **电源滤波**: 建议在电池输入端并联 100μF 电解电容和 0.1μF 瓷片电容

---

## 📁 项目结构

```
balanced-car-pio/
├── src/                      # 源代码目录
│   ├── app/                  # 应用层（业务逻辑）
│   │   ├── control/          # 控制算法（PID 平衡控制）
│   │   ├── storage/          # 数据存储
│   │   ├── system/           # 系统初始化
│   │   ├── task/             # 任务调度
│   │   ├── ui/               # 用户界面
│   │   ├── main.c            # 程序入口
│   │   └── stm32f10x_it.c    # 中断服务函数
│   │
│   ├── config/               # 配置文件
│   │   ├── bsp_config.h      # 板级硬件配置
│   │   ├── debug_config.h    # 调试配置
│   │   └── system_config.h   # 系统全局配置
│   │
│   ├── drivers/              # 硬件驱动层
│   │   ├── Communication/    # 通信模块（蓝牙串口）
│   │   ├── Display/          # 显示模块（OLED）
│   │   ├── IMU/              # 惯性测量单元（MPU6050）
│   │   ├── Input/            # 输入设备（按键、编码器）
│   │   ├── Output/           # 输出设备（电机、PWM）
│   │   └── Sensors/          # 传感器（DHT11）
│   │
│   ├── system/               # 系统底层驱动
│   │   └── stm32f103c8t6_minidev/
│   │       ├── timer.c       # 定时器驱动
│   │       ├── usart.c       # 串口驱动
│   │       ├── i2c.c         # I2C 驱动
│   │       └── ...
│   │
│   └── utils/                # 通用工具库
│       ├── PID/              # PID 控制算法
│       ├── buffer/           # 缓冲区管理
│       └── debug/            # 调试工具
│
├── lib/                      # 第三方库
│   ├── CMSIS/                # ARM Cortex-M 抽象层
│   └── STM32F10x_StdPeriph_Driver/  # 标准外设库
│
├── reports/                  # 项目报告文档
│   ├── ARCHITECTURE_ANALYSIS_REPORT.md  # 架构分析报告
│   └── BUILD_SUCCESS_REPORT.md
│
├── test/                     # 测试代码
├── platformio.ini            # PlatformIO 配置文件
├── CODING_STANDARD.md        # 代码规范文档
└── README.md                 # 本文件
```

---

## 🎯 使用说明

### 开机流程

1. **上电**: 打开电源开关，系统开始启动
2. **自检**: LED 闪烁，系统进行硬件自检
3. **校准**: 将平衡车保持竖直静止，系统进行陀螺仪零偏校准（约 3 秒）
4. **待命**: OLED 显示主菜单，等待启动命令
5. **启动**: 按下 KEY1 键，电机使能，进入平衡状态

### 控制方式

#### 蓝牙遥控器（推荐）

使用手机蓝牙调试 APP（如 "BlueSerial"）：

```
摇杆数据包格式: [joystick,LH,LV,RH,RV]

参数说明:
- LH: 左手柄横向值 (-50 ~ 50)
- LV: 左手柄纵向值 (-50 ~ 50) → 控制前后移动
- RH: 右手柄横向值 (-50 ~ 50) → 控制左右转向
- RV: 右手柄纵向值 (-50 ~ 50)

示例:
[joystick,0,30,0,0]  → 前进
[joystick,0,-20,0,0] → 后退
[joystick,0,0,25,0]  → 左转
[joystick,0,0,-25,0] → 右转
```

#### 按键控制

| 按键 | 短按 | 长按 (2s) |
|------|------|-----------|
| KEY1 | 启动/停止平衡 | 进入校准模式 |
| KEY2 | 切换显示页面 | 恢复出厂设置 |

### OLED 菜单结构

```
主菜单
├── 运行状态
│   ├── 电池电压
│   ├── 车身角度
│   └── 电机转速
├── PID 调节
│   ├── 直立环 P
│   ├── 直立环 D
│   ├── 速度环 P
│   └── 转向环 P
├── 传感器数据
│   ├── 陀螺仪
│   ├── 加速度计
│   └── 互补滤波角度
└── 系统设置
    ├── 保存参数
    └── 恢复默认
```

---

## 🔍 调试指南

### 常见问题排查

#### 问题 1: 编译失败

```bash
# 错误信息：找不到头文件
# 解决方案：
pio run --target clean
pio run

# 检查 platformio.ini 中的 include 路径配置
```

#### 问题 2: 烧录失败

```bash
# 错误信息：No target found
# 解决方案：
# 1. 检查调试器连接是否牢固
# 2. 确认开发板供电正常
# 3. 尝试更换 USB 端口
# 4. 检查 upload_protocol 配置是否正确
```

#### 问题 3: 车辆无法平衡

```bash
# 可能原因及解决：
# 1. MPU6050 通信失败 → 检查 I2C 接线
# 2. 电机相位错误 → 交换 AIN1/AIN2 或 BIN1/BIN2
# 3. PID 参数不合适 → 通过菜单调整 PID
# 4. 编码器方向错误 → 检查编码器计数正负
```

### 日志查看

```bash
# 打开串口监视器（波特率 115200）
pio device monitor --baud 115200

# 或使用 PlatformIO 工具栏的 "Serial Monitor" 按钮
```

### 在线调试

```bash
# 启动调试会话（需要调试器支持）
pio debug

# VSCode 中设置断点，使用 Debug 面板控制执行
```

---

## 📊 性能指标

### 控制环路频率

| 环路 | 频率 | 周期 |
|------|------|------|
| 直立环（内环） | 200Hz | 5ms |
| 速度环（外环） | 25Hz | 40ms |
| 转向环 | 25Hz | 40ms |

### 资源占用

| 资源 | 使用量 | 总量 | 利用率 |
|------|--------|------|--------|
| Flash | 45KB | 64KB | 70% |
| SRAM | 12KB | 20KB | 60% |
| 系统时钟 | 72MHz | 72MHz | 100% |

---

## 🤝 贡献指南

欢迎提交 Issue 和 Pull Request！

### 开发流程

1. Fork 本仓库
2. 创建功能分支 (`git checkout -b feature/amazing-feature`)
3. 提交更改 (`git commit -m 'feat: add amazing feature'`)
4. 推送到分支 (`git push origin feature/amazing-feature`)
5. 创建 Pull Request

### 代码规范

请阅读并遵守 [CODING_STANDARD.md](CODING_STANDARD.md)

---

## 📄 许可证

本项目采用 MIT 许可证 - 查看 [LICENSE](LICENSE) 文件了解详情

---

## 👥 作者

- **Eureka** - 主要作者

感谢所有贡献者！

---

## 🙏 致谢

- [STM32CubeMX](https://www.st.com/en/development-tools/stm32cubemx.html) - 代码生成工具
- [PlatformIO](https://platformio.org/) - 开发环境
- [MPU6050 库](https://github.com/jrowberg/i2cdevlib) - I2C 设备库

---

## 📞 联系方式

如有问题，请通过以下方式联系：

- 提交 GitHub Issue
- 发送邮件至：your-email@example.com

---

**最后更新**: 2026-03-19  
**项目状态**: 🟢 活跃开发中
