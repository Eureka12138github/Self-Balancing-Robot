/**
 ******************************************************************************
 * @file    delay.c
 * @author  Eureka
 * @brief   基于 SysTick 的多级延时与系统时间管理（适用于 STM32F10x）
 *
 * @note    - 提供 us/ms/s 三级阻塞延时
 *          - 维护全局系统滴答计数器（1ms 精度）
 *          - 支持非阻塞超时检测（通过 SysTick_Get()）
 *          - 所有延时函数均为阻塞式，不可在中断服务程序中调用
 *          - s_tick 使用 uint32_t，约每 49.7 天回绕一次，无需手动清零
 ******************************************************************************
 */

#include "stm32f10x.h"
#include "delay.h"

/**
 * @brief  系统滴答计数器（1ms 自增）
 * @note   - 由 SysTick_Handler 自动维护
 *         - 无符号 32 位整数，溢出后自动回绕（约 49.7 天）
 *         - 利用无符号减法特性，超时判断在回绕前后均正确
 */
static volatile uint32_t s_tick = 0;

/**
 * @brief  SysTick 中断服务函数
 * @note   每 1ms 触发一次，用于递增系统时间基准
 */
void SysTick_Handler(void)
{
    s_tick++;
}

/**
 * @brief  初始化延时系统
 * @note   配置 SysTick 为 1ms 中断周期（基于 SystemCoreClock）
 * @retval None
 */
void Delay_Init(void)
{
    /* 配置 SysTick 为 1ms 中断（SystemCoreClock / 1000） */
    if (SysTick_Config(SystemCoreClock / 1000)) {
        /* 配置失败：进入死循环（可根据需求替换为错误处理） */
        while (1);
    }
}

/**
 * @brief  微秒级阻塞延时（基于 SysTick 精确实现）
 * @param  us: 延时时长，单位：微秒（μs）
 * @note   - 基于 SysTick 计数器，精度高
 *         - 适用于所有需要精确微秒延时的场景（如 DHT11、I2C 等）
 *         - 不依赖编译器优化等级
 */
void Delay_us(uint32_t us)
{
    if (us == 0) return;
    
    // 72MHz 下，每个计数 = 1/72 μs ≈ 0.0139 μs
    // 需要的计数值 = us * 72
    uint32_t start = SysTick->VAL;
    uint32_t ticks_needed = us * 72;
    
    // 等待足够的计数
    while (1) {
        uint32_t current = SysTick->VAL;
        uint32_t elapsed;
        
        // 计算经过的计数（处理回绕）
        if (current < start) {
            elapsed = start - current;
        } else {
            elapsed = start + (SysTick->LOAD - current) + 1;
        }
        
        if (elapsed >= ticks_needed) {
            break;
        }
    }
}

/**
 * @brief  毫秒级阻塞延时
 * @param  ms: 延时时长，单位：毫秒（ms）
 * @note   - 基于 SysTick 系统滴答
 *         - 支持长时间延时（最大约 49.7 天）
 *         - 自动处理 s_tick 回绕问题
 *         - 不可在中断服务程序中调用（会阻塞中断响应）
 */
void Delay_ms(uint32_t ms)
{
    if (ms == 0) return;
    
    uint32_t start = s_tick;
    while ((s_tick - start) < ms) {
        /* 空循环等待，可在此加入 __WFI() 进入睡眠以降低功耗 */
    }
}

/**
 * @brief  秒级阻塞延时
 * @param  s: 延时时长，单位：秒（s）
 * @note   - 通过多次调用 Delay_ms(1000) 实现
 *         - 同样支持长时间延时
 */
void Delay_s(uint32_t s)
{
    while (s--) {
        Delay_ms(1000);
    }
}

/**
 * @brief  获取系统运行时间（自启动以来的毫秒数）
 * @retval 当前系统滴答计数值（单位：ms）
 * @note   - 可用于非阻塞超时检测，例如：
 *           uint32_t start = SysTick_Get();
 *           while (!condition && !Delay_Check(start, 1000)) { ... }
 */
uint32_t SysTick_Get(void)
{
    return s_tick;
}
