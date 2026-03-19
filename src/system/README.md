# System 目录说明

## 📋 层次定位

本目录 (`src/system/`) 包含**底层硬件抽象驱动**，提供芯片级的外设操作接口。

### 架构层次

```
┌─────────────────────────────────────────┐
│     Application Layer (src/app/)        │  ← 应用业务逻辑
└─────────────┬───────────────────────────┘
              │ uses
              ▼
┌─────────────────────────────────────────┐
│      Driver Layer (src/drivers/)        │  ← 设备驱动（基于 system/构建）
└─────────────┬───────────────────────────┘
              │ uses
              ▼
┌─────────────────────────────────────────┐
│      System Layer (src/system/)         │  ← 通用外设驱动（本目录）
└─────────────────────────────────────────┘
```

---

## 🗂️ 目录内容

### 核心驱动模块

| 文件 | 功能 | 层级 |
|------|------|------|
| `i2c.c/h` | 软件模拟 I2C 总线驱动 | 底层总线驱动 |
| `timer.c/h` | 硬件定时器驱动 | 底层外设驱动 |
| `usart.c/h` | 串口通信驱动 | 底层外设驱动 |
| `delay.c/h` | 延时函数 | 基础工具 |
| `flash.c/h` | Flash 存储驱动 | 底层外设驱动 |
| `rtc.c/h` | 实时时钟驱动 | 底层外设驱动 |
| `iwdg.c/h` | 独立看门狗驱动 | 系统保护 |

---

## 🔧 依赖规则

### ✅ 允许的依赖方向

```
drivers/*  →  system/*   (允许)
app/*      →  system/*   (允许，但不推荐直接依赖)
```

### ❌ 禁止的依赖方向

```
system/*   →  drivers/*  (禁止！)
system/*   →  app/*      (禁止！)
```

**原因**: 
- `system/` 是最底层，不应该依赖上层模块
- 保持单向依赖，避免循环引用

---

## 📝 使用示例

### 在驱动程序中使用 I2C

```c
// src/drivers/Display/OLED.c
#include "i2c.h"  // ← 包含 system 层的通用 I2C 驱动

void OLED_WriteCommand(uint8_t cmd)
{
    I2C_Start_Instance(0);      // 使用 system/提供的接口
    I2C_SendByte_Instance(0, 0x00);
    I2C_SendByte_Instance(0, cmd);
    I2C_Stop_Instance(0);
}
```

### 在应用层使用（推荐方式）

```c
// src/app/main.c
#include "System_Init.h"  // ← 推荐：通过初始化文件间接使用
#include "OLED.h"         // ← 使用驱动层封装后的接口

int main(void)
{
    System_Init();  // 初始化所有底层驱动
    OLED_Init();    // 初始化 OLED（内部已包含 I2C 初始化）
    
    while (1) {
        OLED_ShowString(1, 1, "Hello");
    }
}
```

---

## 🔄 未来演进

### 短期（1-2 个月）

保持现有结构，但会：
- ✅ 添加清晰的文档说明（本文档）
- ✅ 明确依赖规则
- ✅ 规范命名和接口

### 中期（3-6 个月）

计划重构成正式的 **HAL 层**:

```bash
# 目录重命名
mv src/system src/hal

# 或保持兼容，添加新目录
mkdir src/hal
cp src/system/* src/hal/  # 迁移并改进
```

**HAL 层特点**:
- 更清晰的接口抽象
- 支持运行时配置
- 更好的可移植性

---

## 🎯 设计原则

### 1. 通用性优先

`system/` 中的驱动应该是**通用的、与具体设备无关**的。

**✅ 正确示例**:
```c
// system/i2c.c - 提供通用 I2C 操作
void I2C_SendByte_Instance(uint8_t id, uint8_t byte);
```

**❌ 错误示例**:
```c
// system/i2c.c - 不应该包含特定设备逻辑
void OLED_SendCommand(...) {  // 这应该在 drivers/OLED.c 中
    // ...
}
```

### 2. 最小依赖

`system/` 应该只依赖:
- CMSIS (ARM Cortex-M 标准)
- STM32 标准外设库
- 本目录内的其他模块

**不应该依赖**:
- `drivers/` 中的设备驱动
- `app/` 中的应用逻辑

### 3. 接口稳定

一旦接口确定，应保持稳定，避免频繁改动影响上层代码。

如需修改接口:
1. 评估影响范围
2. 通知相关开发者
3. 同步更新所有调用方
4. 更新文档

---

## 📚 相关文档

- [ARCHITECTURE_ANALYSIS_REPORT.md](../reports/ARCHITECTURE_ANALYSIS_REPORT.md) - 整体架构分析
- [ARCHITECTURE_FIX_RECORD.md](../reports/ARCHITECTURE_FIX_RECORD.md) - 层次问题详细说明
- [OPTIMIZATION_ROADMAP.md](../reports/OPTIMIZATION_ROADMAP.md) - 未来优化路线

---

## ❓ 常见问题

### Q: `system/` 和 `drivers/` 有什么区别？

**A**: 
- `system/`: 提供**通用外设驱动**（I2C、UART、Timer），接近硬件寄存器
- `drivers/`: 提供**具体设备驱动**（OLED、MPU6050、Motor），基于 system/构建

**类比**:
```
system/i2c.c  ≈  Linux 内核的 I2C 子系统驱动
drivers/OLED.c ≈  Linux 的具体 LCD 显示屏驱动
```

### Q: 为什么不在 `drivers/` 中直接实现 I2C？

**A**: 
- **代码复用**: 多个设备（OLED、MPU6050）都需要 I2C，统一实现避免重复
- **一致性**: 统一的 I2C 接口保证所有设备行为一致
- **维护性**: 修改 I2C 实现时只需改一处

### Q: 可以直接在 `main.c` 中包含 `system/i2c.h` 吗？

**A**: 
- **可以，但不推荐**
- 推荐做法：通过驱动层间接使用
  ```c
  // ❌ 不推荐
  #include "system/i2c.h"
  
  // ✅ 推荐
  #include "drivers/OLED.h"  // OLED.h 内部已包含 i2c.h
  ```

---

**最后更新**: 2026-03-19  
**维护者**: Eureka & Lingma
**下次审查**: 当进行 HAL 层重构时
