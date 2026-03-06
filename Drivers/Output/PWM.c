#include "PWM.h"
#include "stm32f10x.h"                  // Device header

/**
 ******************************************************************************
 * @file    PWM.h
 * @brief   PWM 输出驱动（基于 STM32F10x 标准外设库）
 *
 * @note    【硬件连接说明】
 *          本驱动使用定时器的默认引脚映射（无重映射）。
 *          默认配置：
 *            - 定时器：TIM2
 *            - 通道：CH1（对应 TIM_OC1 / CCR1）、CH2（对应 TIM_OC2 / CCR2）
 *            - 引脚：PA0（TIM2_CH1 的默认引脚）、PA1（TIM2_CH2 的默认引脚）
 *          所有硬件资源通过宏定义集中管理，修改即可适配其他组合。
 ******************************************************************************
 */

// =============================================================================
// 【用户配置区】—— 修改以下宏即可适配不同引脚/定时器
// ⚠️ 重要：所选引脚必须与【通道】匹配！
//         例如：TIM2_CH1 → PA0, TIM3_CH1 → PA6, TIM4_CH1 → PB6
// =============================================================================

// 定时器选择（必须支持 PWM 输出）
#define PWM_TIMER               TIM2 
#define PWM_TIMER_RCC           RCC_APB1Periph_TIM2     // TIM2/TIM3/TIM4 属于 APB1，TIM1 属于 APB2

// PWM 输出引脚配置（必须与所选定时器通道的默认引脚一致）
#define PWM_GPIO_PORT            GPIOA
#define PWM_CH1_PIN     GPIO_Pin_0   // TIM2_CH1 → PA0
#define PWM_CH2_PIN     GPIO_Pin_1   // TIM2_CH2 → PA1

//GPIO 时钟
#define PWM_GPIO_RCC        RCC_APB2Periph_GPIOA



/**
 * @brief 初始化 PWM 输出（使用定时器默认引脚，无重映射）
 * @note  同时配置 TIMx_CH1 和 TIMx_CH2 为 PWM1 模式，
 *        引脚由硬件默认映射决定（如 TIM2_CH1 → PA0, TIM2_CH2 → PA1）
 */
void PWM_Init(void)
{
    // 1. 使能定时器时钟（APB1）
    RCC_APB1PeriphClockCmd(PWM_TIMER_RCC, ENABLE);

    // 2. 使能 GPIO 时钟（APB2）
    RCC_APB2PeriphClockCmd(PWM_GPIO_RCC, ENABLE);

    // 3. 配置 PWM 引脚为复用推挽输出（必须！）
    GPIO_InitTypeDef GPIO_InitStruct;
    GPIO_InitStruct.GPIO_Mode = GPIO_Mode_AF_PP;    // 复用推挽
    GPIO_InitStruct.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStruct.GPIO_Pin = PWM_CH1_PIN | PWM_CH2_PIN;
    GPIO_Init(PWM_GPIO_PORT, &GPIO_InitStruct);

    // 4. 计算预分频值（PSC），使 PWM 频率 = PWM_FREQ_HZ
    // 公式：PWM_Freq = SystemCoreClock / ((PSC + 1) * (ARR + 1))
    uint32_t prescaler = SystemCoreClock / (PWM_FREQ_HZ * PWM_RESOLUTION) - 1;

    // 5. 配置定时器时基单元
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStruct;
    TIM_TimeBaseStruct.TIM_ClockDivision = TIM_CKD_DIV1;
    TIM_TimeBaseStruct.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStruct.TIM_Period = PWM_RESOLUTION - 1;      // ARR
    TIM_TimeBaseStruct.TIM_Prescaler = (uint16_t)prescaler;  // PSC
    TIM_TimeBaseStruct.TIM_RepetitionCounter = 0;            // 仅高级定时器使用
    TIM_TimeBaseInit(PWM_TIMER, &TIM_TimeBaseStruct);

    // 6. 配置通道 1 为 PWM 模式
    TIM_OCInitTypeDef TIM_OCStruct;
    TIM_OCStructInit(&TIM_OCStruct);                         // 初始化默认值
    TIM_OCStruct.TIM_OCMode = TIM_OCMode_PWM1;
    TIM_OCStruct.TIM_OCPolarity = TIM_OCPolarity_High;
    TIM_OCStruct.TIM_OutputState = TIM_OutputState_Enable;
    TIM_OCStruct.TIM_Pulse = 0;                              // 初始占空比 = 0%
    TIM_OC1Init(PWM_TIMER, &TIM_OCStruct);                   // 初始化 CH1
    TIM_OC2Init(PWM_TIMER, &TIM_OCStruct);                   // 初始化 CH2
    // 7. 使能定时器
    TIM_Cmd(PWM_TIMER, ENABLE);
}

/**
 * @brief 设置 PWM 占空比（通道 1）
 * @param Compare: 占空比数值，有效范围 0 ~ PWM_RESOLUTION
 * @note  实际占空比 = Compare / PWM_RESOLUTION * 100%
 *        超出范围的值会被静默限制在合法区间内。
 */
void PWM_SetCompare1(uint16_t Compare)
{
    if (Compare > PWM_RESOLUTION) {
        Compare = PWM_RESOLUTION;
    }
    TIM_SetCompare1(PWM_TIMER, Compare);
}

/**
 * @brief 设置 PWM 占空比（通道 2）
 * @param Compare: 占空比数值，有效范围 0 ~ PWM_RESOLUTION
 * @note  实际占空比 = Compare / PWM_RESOLUTION * 100%
 *        超出范围的值会被静默限制在合法区间内。
 */
void PWM_SetCompare2(uint16_t Compare)
{
    if (Compare > PWM_RESOLUTION) {
        Compare = PWM_RESOLUTION;
    }
    TIM_SetCompare2(PWM_TIMER, Compare);
}
