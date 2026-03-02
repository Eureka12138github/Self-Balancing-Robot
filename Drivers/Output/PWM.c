#include "pwm.h"

/**
 * @brief 初始化 PWM 输出（使用定时器默认引脚，无重映射）
 * @note  配置 TIMx_CH1 为 PWM1 模式，引脚自动由硬件默认映射决定
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
    GPIO_InitStruct.GPIO_Pin = PWM_PIN;
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
