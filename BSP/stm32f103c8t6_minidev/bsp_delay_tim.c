/**
 ******************************************************************************
 * @file    Delay.c
 * @author  Eureka
 * @brief   基于 TIM3 的高精度阻塞式延时函数实现（适用于 STM32F103 系列）
 *
 * @note    - 本模块使用 TIM3 定时器，不依赖 SysTick，不影响系统滴答中断
 *          - 要求系统主频为 72MHz（标准配置）
 *          - PSC = 71 → 计数频率 = 72MHz / (71+1) = 1MHz → 1 tick = 1μs
 *          - 所有延时函数均为阻塞式，不可在中断服务程序中调用
 ******************************************************************************
 */

#include "Delay.h"

/**
 * @brief  初始化 TIM3 用于微秒级延时
 * @note   配置 TIM3 为向上计数模式，时钟源为 72MHz，
 *         预分频 PSC=71 → 计数频率 1MHz（1μs/tick）
 * @param  None
 * @retval None
 */
void Delay_TIM_Init(void)
{
    /* 使能 TIM3 时钟（位于 APB1 总线） */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);

    /* 配置 TIM3 基本参数 */
    TIM_TimeBaseInitTypeDef TIM_InitStruct;
    TIM_InitStruct.TIM_Period        = 0xFFFF;          // 自动重装载值设为最大（65535）
    TIM_InitStruct.TIM_Prescaler     = 71;              // 预分频值：71 → 实际分频系数 = 72
    TIM_InitStruct.TIM_ClockDivision = 0;               // 时钟分频因子（不使用）
    TIM_InitStruct.TIM_CounterMode   = TIM_CounterMode_Up; // 向上计数模式

    TIM_TimeBaseInit(TIM3, &TIM_InitStruct);

    /* 启动定时器（不启用中断） */
    TIM_Cmd(TIM3, ENABLE);
}

/**
 * @brief  微秒级阻塞延时
 * @param  us: 延时时长，单位：微秒（μs）
 *         - 有效范围：1 ～ 1,000,000 μs（即最大 1 秒）
 *         - 若传入 0 或超出范围，函数直接返回
 * @retval None
 * @note   - 内部使用 TIM3->CNT 轮询实现
 *         - 单次最多延时 65535 μs（受限于 16 位 ARR），自动分段处理更长延时
 *         - 不可在中断服务程序中调用（非可重入）
 */
void Delay_us(uint32_t us)
{
    /* 参数合法性检查 */
    if (us == 0 || us > 1000000) {
        return;
    }

    /* 分段处理：每次最多延时 65535 μs（ARR 为 16 位寄存器） */
    while (us > 0xFFFF) {
        TIM_SetAutoreload(TIM3, 0xFFFF);    // 设置 ARR = 65535
        TIM_SetCounter(TIM3, 0);            // 清零计数器
        while (TIM_GetCounter(TIM3) < 0xFFFF) {
            /* 等待计数器达到 65535（耗时 65535 μs） */
        }
        us -= 0xFFFF;
    }

    /* 处理剩余部分（us ≤ 65535） */
    TIM_SetAutoreload(TIM3, (uint16_t)us); // 设置 ARR = us（关键：不再减 1！）
    TIM_SetCounter(TIM3, 0);               // 清零计数器
    while (TIM_GetCounter(TIM3) < us) {
        /* 等待计数器达到 us（实际经过 us 个 1μs 周期） */
    }
}

/**
 * @brief  毫秒级阻塞延时
 * @param  ms: 延时时长，单位：毫秒（ms）
 * @retval None
 */
void Delay_ms(uint32_t ms)
{
    while (ms--) {
        Delay_us(1000);  // 1ms = 1000μs
    }
}

/**
 * @brief  秒级阻塞延时
 * @param  s: 延时时长，单位：秒（s）
 * @retval None
 */
void Delay_s(uint32_t s)
{
    while (s--) {
        Delay_ms(1000);  // 1s = 1000ms
    }
}