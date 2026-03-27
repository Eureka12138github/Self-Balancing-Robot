# 项目编码规范

## 📋 目的

本规范旨在统一平衡车项目的代码风格，提高代码可读性、可维护性和团队协作效率。

**所有项目参与者必须严格遵守以下规范。**

---

## 🎨 一、命名约定

### 1.1 文件命名

```bash
# ✅ 正确示例
hall_encoder.c/h      # 小写字母 + 下划线
system_config.h       # 小写字母 + 下划线
PID.h                 # 缩写可用大写
stm32f10x_conf.h      # 芯片型号保持原有风格

# ❌ 错误示例
HallEncoder.c         # 驼峰式文件名
system-config.h       # 连字符可能导致编译问题
```

**规则：**
- 源文件和头文件统一使用 **小写字母 + 下划线**
- 文件名应清晰反映模块功能
- 避免使用空格和特殊字符

### 1.2 类型命名

```c
// ✅ 正确示例
typedef enum {
    TASK_STATE_IDLE,
    TASK_STATE_READY
} task_state_t;           // 枚举类型加 _t 后缀

typedef struct {
    int16_t target;
    int16_t actual;
} pid_config_t;           // 结构体类型加 _t 后缀

// ❌ 错误示例
typedef enum {
    TaskStateIdle,        // 驼峰式枚举
} TaskState;              // 缺少_t 后缀
```

**规则：**
- 自定义类型使用 **snake_case + `_t` 后缀**
- 枚举值使用 **UPPER_CASE**（全大写 + 下划线）

### 1.3 变量命名

```c
// ✅ 正确示例
int16_t encoder_value;    // 局部变量：snake_case
float pid_output;
uint32_t task_counter;

static bool g_initialized = false;  // 全局静态变量：g_前缀
static int16_t g_motor_speed;

const uint8_t* p_buffer;  // 指针变量：p_前缀（可选）

// ❌ 错误示例
int16_t EncoderValue;     // 驼峰式局部变量
float PIDOutput;
static bool initialized;  // 全局变量缺少 g_前缀
```

**规则：**
- 局部变量：**snake_case**（小写 + 下划线）
- 全局静态变量：`g_` 前缀 + snake_case
- 指针变量：可选 `p_` 前缀
- 避免单字母变量（循环计数器 i,j,k 除外）

### 1.4 常量与宏定义

```c
// ✅ 正确示例
#define MAX_MOTOR_SPEED   (100)           // 常量宏：UPPER_CASE
#define DEBOUNCE_TICKS    (20U)           // 无符号数加 U 后缀
#define PI                (3.1415926f)    // 浮点数加 f 后缀

// 带参数的宏
#define MIN(a, b)         (((a) < (b)) ? (a) : (b))

// ❌ 错误示例
#define MaxSpeed 100      // 非全大写
#define debounce_ticks 20 // 小写易与变量混淆
```

**规则：**
- 宏定义和常量：**UPPER_CASE**（全大写 + 下划线）
- 复杂表达式用括号包裹
- 数值字面量添加类型后缀（U、f、L 等）

### 1.5 函数命名

```c
// ✅ 正确示例 - 方案 A: PascalCase（推荐）
void Hall_Encoder_Init(void);
int16_t Hall_Encoder_Get(EncoderId_t id);
err_code_t PID_Update(PID_t* pid);

// ✅ 正确示例 - 方案 B: snake_case
void hall_encoder_init(void);
int16_t hall_encoder_get(encoder_id_t id);
err_code_t pid_update(pid_t* pid);

// ❌ 错误示例
void Hall_Encoder_init(void);     // 大小写混用
int16_t getHallEncoder(void);     // 动词前置风格不一致
```

**规则：**
- **团队需统一选择 PascalCase 或 snake_case 其中之一**
- 函数名应清晰表达功能（动词 + 名词）
- 初始化函数：`Module_Init()`
- 读写函数：`Module_Read()`, `Module_Write()`
- 布尔查询：`Module_IsReady()`, `Module_HasData()`

---

## 📝 二、注释规范

### 2.1 文件头注释

```c
/**
 * @file hall_encoder.c
 * @brief 霍尔编码器驱动程序
 * 
 * 本模块实现 STM32F103C8T6 正交编码器接口（QEI）的
 * 封装，支持双路编码器同时采集。
 * 
 * @author 张三
 * @date 2026-03-19
 * @version 1.0
 * 
 * @par 硬件连接:
 * - 左编码器：PA6(TIM3_CH1), PA7(TIM3_CH2)
 * - 右编码器：PB6(TIM4_CH1), PB7(TIM4_CH2)
 * 
 * @par 版本历史:
 * - v1.0 (2026-03-19): 初始版本
 * - v1.1 (2026-03-20): 添加速度转换功能
 */
```

### 2.2 函数注释

```c
/**
 * @brief 初始化霍尔编码器接口
 * 
 * 配置 TIM3 和 TIM4 为正交编码器模式，
 * 自动检测 A/B 相脉冲并计数。
 * 
 * @attention 
 * - 调用此函数前需确保系统时钟已初始化
 * - 此函数会清零计数器
 * 
 * @note 
 * - 编码器最大计数值：32767
 * - 计数器溢出后会自动回绕
 * 
 * @param 无
 * @return 无
 * 
 * @par 使用示例:
 * @code
 * Hall_Encoder_Init();
 * int16_t speed = Hall_Encoder_Get(ENCODER_LEFT);
 * @endcode
 */
void Hall_Encoder_Init(void)
{
    // ...
}
```

### 2.3 参数和返回值注释

```c
/**
 * @brief 读取编码器速度值
 * 
 * @param[in]  id      编码器通道标识符
 * @param[out] value   存储读取到的速度值
 * 
 * @return 错误码
 * @retval ERR_OK      读取成功
 * @retval ERR_INVALID_PARAM  参数错误
 * @retval ERR_TIMEOUT 读取超时
 * 
 * @attention 此函数会清除计数器标志位
 */
err_code_t Hall_Encoder_Get_Speed(EncoderId_t id, int16_t* value)
{
    // ...
}
```

### 2.4 行内注释

```c
// ✅ 正确示例
uint32_t timeout = 1000;
while (--timeout > 0) {
    if (flag_set) {
        break;  // 提前退出：检测到标志位
    }
}

// 计算目标角度（单位：度）
target_angle = gyro_data * 0.05f + acc_data * 0.95f;

/* 
 * 注意：以下延时必须精确到微秒级
 * 否则会影响 DHT11 通信时序
 */
Delay_us(18);

// ❌ 错误示例
uint32_t t = 1000;  // 临时变量 t
while (t-- > 0) {   // 循环等待
    if (f) break;   // 啥意思？
}
```

**规则：**
- 注释应解释 **"为什么"** 而非 **"是什么"**
- 复杂算法需说明数学原理或参考来源
- 避免无意义的重复注释

---

## 🏛️ 三、代码组织

### 3.1 头文件保护

```c
// ✅ 推荐方式：#ifndef 保护
#ifndef HALL_ENCODER_H
#define HALL_ENCODER_H

// 代码内容

#endif /* HALL_ENCODER_H */

// ✅ 现代编译器支持：#pragma once
#pragma once

// 代码内容

// ❌ 不推荐：无保护
// 代码内容
```

### 3.2 Include 顺序

```c
// 1. 标准库头文件
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

// 2. CMSIS/芯片头文件
#include "stm32f10x.h"

// 3. 项目模块头文件（按依赖层级排序）
#include "hal_gpio.h"      // 硬件抽象层
#include "bsp_pins.h"      // 板级配置
#include "hall_encoder.h"  // 驱动层
#include "control.h"       // 应用层

// 4. 空行分隔
// 5. 本模块对应的头文件（如果是 .c 文件）
#include "hall_encoder.c"
```

### 3.3 代码分块

```c
/* ======================== 宏定义 ======================== */
#define MAX_SPEED   (100)
#define MIN_SPEED   (-100)


/* ======================== 类型定义 ======================== */
typedef struct {
    int16_t target;
    int16_t actual;
} control_data_t;


/* ======================== 全局变量 ======================== */
static int16_t g_motor_speed = 0;
static bool g_initialized = false;


/* ======================== 函数声明 ======================== */
static void Motor_Config_GPIO(void);
static void Motor_Config_TIMER(void);


/* ======================== 导出函数 ======================== */
void Motor_Init(void)
{
    Motor_Config_GPIO();
    Motor_Config_TIMER();
}
```

---

## 🔧 四、最佳实践

### 4.1 错误处理

```c
// ✅ 推荐：统一错误处理机制
typedef enum {
    ERR_OK = 0,
    ERR_INVALID_PARAM,
    ERR_TIMEOUT,
    ERR_NOT_INITIALIZED,
    ERR_HARDWARE_FAULT
} err_code_t;

err_code_t Sensor_Read(float* value)
{
    if (value == NULL) {
        return ERR_INVALID_PARAM;
    }
    
    if (!g_initialized) {
        return ERR_NOT_INITIALIZED;
    }
    
    // ... 读取操作
    
    return ERR_OK;
}

// 使用示例
err_code_t ret = Sensor_Read(&data);
if (ret != ERR_OK) {
    Error_Handler(ret);
}
```

### 4.2 断言使用

```c
// ✅ 推荐：调试阶段检查
#include "my_assert.h"

void Motor_Set_Speed(int16_t speed)
{
    MY_ASSERT(speed >= -100 && speed <= 100);
    
    // ... 设置逻辑
}

// ❌ 不推荐：生产环境使用 assert
#include <assert.h>
assert(speed >= -100);  // 可能与 NDEBUG 宏冲突
```

### 4.3 内存管理

```c
// ✅ 推荐：栈上分配（嵌入式首选）
void process_data(void)
{
    uint8_t buffer[256];  // 栈上分配
    // ...
}

// ⚠️ 谨慎：堆上分配（需检查返回值）
uint8_t* buffer = pvPortMalloc(256);
if (buffer == NULL) {
    Error_Handler();
}
// ... 使用后必须释放
vPortFree(buffer);

// ❌ 禁止：内存泄漏
void leak_memory(void)
{
    uint8_t* buffer = malloc(256);
    // 忘记 free
}
```

### 4.4 中断处理

```c
// ✅ 推荐：快速退出原则
volatile uint8_t g_rx_flag = 0;
volatile uint8_t g_rx_data = 0;

void USART1_IRQHandler(void)
{
    if (USART_GetITStatus(USART1, USART_IT_RXNE) != RESET) {
        g_rx_data = USART_ReceiveData(USART1);
        g_rx_flag = 1;  // 置标志位后立即退出
    }
}

// 主循环处理
while (1) {
    if (g_rx_flag) {
        g_rx_flag = 0;
        Process_Data(g_rx_data);  // 耗时操作放在主循环
    }
}

// ❌ 不推荐：中断中执行耗时操作
void USART1_IRQHandler(void)
{
    // 错误：在中断中执行复杂逻辑
    Complex_Algorithm();
    Delay_ms(100);
}
```

---

## 📦 五、Git 提交规范

### 5.1 提交信息格式

```
<type>(<scope>): <subject>

<body>

<footer>
```

### 5.2 Type 类型

| 类型 | 说明 | 示例 |
|------|------|------|
| `feat` | 新功能 | feat(motor): 添加电机速度闭环控制 |
| `fix` | Bug 修复 | fix(encoder): 修复编码器溢出错误 |
| `refactor` | 重构（不影响功能） | refactor(ui): 重构菜单渲染逻辑 |
| `docs` | 文档更新 | docs(readme): 添加安装说明 |
| `test` | 测试相关 | test(pid): 添加 PID 参数单元测试 |
| `chore` | 构建/工具配置 | chore(platformio): 更新依赖版本 |
| `perf` | 性能优化 | perf(display): 优化 OLED 刷新率 |
| `style` | 代码格式调整 | style(format): 修正缩进 |

### 5.3 提交示例

```bash
# ✅ 好的提交
git commit -m "feat(motor): 添加电机软启动功能"
git commit -m "fix(encoder): 修复计数器溢出导致的跳变问题"
git commit -m "refactor(hal): 抽象 I2C 接口以支持多实例"

# ❌ 差的提交
git commit -m "更新代码"
git commit -m "修复 bug"
git commit -m "asdfasdf"
```

### 5.4 分支管理

```bash
# 主分支
main/master          # 稳定版本
dev                  # 开发分支

# 功能分支
feature/ui-menu      # UI 菜单功能
feature/ble-control  # 蓝牙控制功能

# 修复分支
fix/encoder-bug      # 编码器 bug 修复
hotfix/critical-fix  # 紧急修复
```

---

## 🧪 六、代码审查清单

### 提交前自检

- [ ] 代码符合本规范要求
- [ ] 添加了必要的注释
- [ ] 通过了编译且无警告
- [ ] 进行了基本功能测试
- [ ] 没有遗留的调试代码（printf、LED 闪烁等）
- [ ] Git 提交信息规范
- [ ] 更新了相关文档

### 审查要点

- [ ] 逻辑正确性
- [ ] 边界条件处理
- [ ] 错误处理完整性
- [ ] 性能影响评估
- [ ] 安全性考虑
- [ ] 可测试性

---

## 📚 七、参考资料

- [Google C++ Style Guide](https://google.github.io/styleguide/cppguide.html)
- [ARM Embedded Coding Standard](https://developer.arm.com/architectures/system-architectures/software-standards/embedded-coding-standard)
- [MISRA C Guidelines](https://www.misra.org.uk/)（汽车电子标准，可参考）

---

## 📝 修订历史

| 版本 | 日期 | 作者 | 变更说明 |
|------|------|------|----------|
| 1.0 | 2026-03-19 | Lingma AI | 初始版本 |

---

**生效日期**: 2026-03-19  
**适用范围**: 平衡车项目所有源代码  
**违规处理**: Code Review 时指出并要求整改
