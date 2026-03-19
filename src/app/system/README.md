# System 模块说明

## 📋 职责定位

本目录 (`src/app/system/`) 包含**系统级初始化模块**，负责统一管理所有硬件和软件的初始化流程。

### 核心文件

| 文件 | 功能 | 说明 |
|------|------|------|
| `System_Init.c/h` | 系统总初始化 | 集中管理所有外设初始化 |
| `system_config.c/h` | 系统全局配置 | 定义全局变量和配置参数 |

---

## 🏗️ 架构层次

```
┌─────────────────────────────────────────┐
│         main.c                          │
│  #include "System_Init.h"               │
└─────────────┬───────────────────────────┘
              │ calls
              ▼
┌─────────────────────────────────────────┐
│    app/system/System_Init.c             │  ← 本目录
│  - Initialize_System()                  │
│  - 调用所有驱动层的 Init 函数            │
└─────────────┬───────────────────────────┘
              │ calls
              ▼
┌─────────────────────────────────────────┐
│    drivers/*.c (Motor, OLED, etc.)      │
│  - Motor_Init()                         │
│  - OLED_Init()                          │
└─────────────────────────────────────────┘
```

---

## 🔧 设计原则

### 1. 单一职责

`System_Init.c` **唯一职责**是协调各模块的初始化，不包含具体业务逻辑。

**✅ 正确示例**:
```c
void Initialize_System(void) {
    Motor_Init();      // 调用电机初始化
    OLED_Init();       // 调用显示初始化
    MPU6050_Init();    // 调用传感器初始化
}
```

**❌ 错误示例**:
```c
void Initialize_System(void) {
    // 不应该在这里写具体的寄存器配置
    GPIO_SetBits(GPIOA, GPIO_Pin_0);  
    RCC->AHBENR |= RCC_AHBENR_GPIOAEN;
}
```

### 2. 集中管理

所有初始化应该在一个地方完成，避免分散在多处。

**好处**:
- ✅ 易于理解和维护
- ✅ 便于修改初始化顺序
- ✅ 减少遗漏的风险

### 3. 避免循环依赖

使用**前向声明**而非直接包含所有头文件。

**当前做法**:
```c
// System_Init.h
#ifndef  SYSTEM_INIT_H
#define  SYSTEM_INIT_H

#include "stm32f10x.h"

// 前向声明 - 减少头文件包含
typedef struct PID_t PID_t;

void Initialize_System(void);

#endif
```

**而不是**:
```c
// ❌ 包含太多头文件，容易导致循环依赖
#include "stm32f10x.h"
#include "system_config.h"
#include "RP.h"
#include "Motor.h"
#include "usart.h"
#include "MPU6050.h"
#include "Delay.h"
#include "BlueSerial.h"
#include "control.h"
```

---

## 📝 使用示例

### 在 main.c 中使用

```c
#include "menu.h"
#include "task_sched.h"
#include "System_Init.h"  // ← 只需包含这个
#include <string.h>
#include <stdlib.h>

int main(void)
{
    SystemInit();              // CMSIS 系统初始化
    SystemClock_Config();      // 时钟配置
    Initialize_System();       // ← 调用统一初始化
    
    while (1) {
        TaskHandler();
        MyOLED_UI_MainLoop();
        IWDG_ReloadCounter();
    }
}
```

### 添加新模块的初始化

如果要添加新的传感器（如 DHT11）：

**步骤 1**: 在 `System_Init.c` 中包含头文件
```c
#include "dht11.h"  // 新增
```

**步骤 2**: 在 `Initialize_System()`中调用初始化
```c
void Initialize_System(void) {
    // ... 其他初始化 ...
    DHT11_Init();  // ← 新增
    // ... 其他初始化 ...
}
```

**步骤 3**: 验证编译通过
```bash
pio run
```

---

## 🔄 与 system/目录的区别

### 本目录 (`app/system/`)

- **职责**: 应用层系统初始化
- **内容**: `System_Init.c`, `system_config.c`
- **特点**: 协调各模块，不直接操作硬件

### `src/system/` 目录

- **职责**: 底层硬件抽象驱动
- **内容**: `i2c.c`, `timer.c`, `usart.c`等
- **特点**: 直接操作寄存器，提供通用驱动

**关系**:
```
main.c 
  → app/system/System_Init.c (协调者)
    → drivers/*.c (设备驱动)
      → src/system/*.c (底层驱动)
```

---

## ⚠️ 注意事项

### 1. 初始化顺序很重要

当前初始化顺序:
```c
void Initialize_System(void) {
    MyOLED_UI_Init(&MainPage);   // 1. 先初始化显示
    Alarm_Init();                 // 2. 警报
    Usart1_Init(115200);          // 3. 串口
    NVIC_PriorityGroupConfig(...);// 4. 中断优先级
    MYIWD_Init(2000);             // 5. 看门狗
    Motor_Init();                 // 6. 电机
    Hall_Encoder_Init();          // 7. 编码器
    MPU6050_Init();               // 8. IMU
    BlueSerial_Init();            // 9. 蓝牙
    LED_Init();                   // 10. LED
    LED1_OFF();
}
```

**不要随意更改顺序**,除非你确定没有依赖关系！

### 2. 避免重复初始化

如果某个模块已经在其他地方初始化了，不要在这里再次调用。

**示例**:
```c
// ❌ 错误：重复初始化
void Initialize_System(void) {
    Usart1_Init(115200);  // 第一次
    // ... 其他代码 ...
    Usart1_Init(115200);  // 第二次 - 错误！
}
```

### 3. 全局变量放在 system_config.c

不要在 `System_Init.c` 中定义全局变量，应该放在`system_config.c`:

**✅ 正确**:
```c
// system_config.c
PID_t anglePID = {0};  // 定义全局变量

// System_Init.c
extern PID_t anglePID;  // 只是引用
anglePID.target = 100;
```

---

## 📚 相关文档

- [ARCHITECTURE_ANALYSIS_REPORT.md](../../reports/ARCHITECTURE_ANALYSIS_REPORT.md) - 整体架构分析
- [main.c](../main.c) - 主程序入口
- [system_config.h](../../config/system_config.h) - 系统配置

---

**最后更新**: 2026-03-19  
**维护者**: Eureka
