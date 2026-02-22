/**
 ******************************************************************************
 * @file    bsp_delay.c
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
#include "bsp_delay.h"

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
 * @brief  微秒级阻塞延时（空循环实现）
 * @param  us: 延时时长，单位：微秒（μs）
 * @note   - 适用于短延时（建议 ≤ 1000μs）
 *         - 基于经验系数（72MHz 下 us * 10 ≈ 1μs），保守设计确保延时不短于请求值
 *         - 不依赖外设，可安全用于任何上下文（包括 ISR）
 *         - 精度受编译器优化等级影响，-O2 下通常足够用于 DHT11/I2C 等场景
 */
void Delay_us(uint32_t us)
{
    if (us == 0) return;
    
    volatile uint32_t i;
    for (i = 0; i < us * 10; i++);  // 保守系数：确保实际延时 ≥ 请求值
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
