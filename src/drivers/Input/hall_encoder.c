#include "hall_encoder.h"
#include "stm32f10x.h"

// =============================================================================
// 【用户配置区】—— 修改此处即可适配不同引脚/定时器
// ⚠️ 必须确保所选引脚与定时器通道的默认映射一致！
// =============================================================================

// 左编码器配置（霍尔编码器，输出 AB 正交信号）
#define HALL_ENC_LEFT_TIMER         TIM3
#define HALL_ENC_LEFT_RCC           RCC_APB1Periph_TIM3
#define HALL_ENC_LEFT_GPIO_PORT     GPIOA
#define HALL_ENC_LEFT_GPIO_RCC      RCC_APB2Periph_GPIOA
#define HALL_ENC_LEFT_PIN_A         GPIO_Pin_6   // TIM3_CH1
#define HALL_ENC_LEFT_PIN_B         GPIO_Pin_7   // TIM3_CH2

// 右编码器配置
#define HALL_ENC_RIGHT_TIMER        TIM4
#define HALL_ENC_RIGHT_RCC          RCC_APB1Periph_TIM4
#define HALL_ENC_RIGHT_GPIO_PORT    GPIOB
#define HALL_ENC_RIGHT_GPIO_RCC     RCC_APB2Periph_GPIOB
#define HALL_ENC_RIGHT_PIN_A        GPIO_Pin_6   // TIM4_CH1
#define HALL_ENC_RIGHT_PIN_B        GPIO_Pin_7   // TIM4_CH2

/**
 * @brief 初始化双路霍尔编码器接口（输出 AB 正交信号）
 * @details 
 *   配置两组硬件编码器接口，利用 STM32 定时器的编码器模式自动计数：
 *   - 方向由 A/B 相位差硬件判断
 *   - 计数值增减由硬件自动完成（无需软件干预）
 *   - 使用最大滤波（0xF）抑制信号抖动
 *
 * @note 硬件连接（默认映射，无重映射）：
 *       - 左编码器：A → HALL_ENC_LEFT_PIN_A, B → HALL_ENC_LEFT_PIN_B
 *       - 右编码器：A → HALL_ENC_RIGHT_PIN_A, B → HALL_ENC_RIGHT_PIN_B
 *       - 引脚配置为上拉输入，防止悬空干扰
 *
 * @warning 
 *   1. 本驱动假设编码器输出标准 AB 正交信号（非三相霍尔）
 *   2. 定时器 ARR 设为 65535（16位满量程），高速场景需配合溢出处理
 */
void Hall_Encoder_Init(void)
{
    /* 1. 使能定时器与GPIO时钟 */
    RCC_APB1PeriphClockCmd(HALL_ENC_LEFT_RCC | HALL_ENC_RIGHT_RCC, ENABLE);
    RCC_APB2PeriphClockCmd(HALL_ENC_LEFT_GPIO_RCC | HALL_ENC_RIGHT_GPIO_RCC, ENABLE);

    /* 2. 配置编码器输入引脚为上拉输入 */
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Mode  = GPIO_Mode_IPU;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;

    // 左编码器引脚
    gpio.GPIO_Pin = HALL_ENC_LEFT_PIN_A | HALL_ENC_LEFT_PIN_B;
    GPIO_Init(HALL_ENC_LEFT_GPIO_PORT, &gpio);

    // 右编码器引脚
    gpio.GPIO_Pin = HALL_ENC_RIGHT_PIN_A | HALL_ENC_RIGHT_PIN_B;
    GPIO_Init(HALL_ENC_RIGHT_GPIO_PORT, &gpio);

    /* 3. 配置定时器通用参数 */
    TIM_TimeBaseInitTypeDef tim_base;
    TIM_ICInitTypeDef       tim_ic;

    tim_base.TIM_ClockDivision     = TIM_CKD_DIV1;
    tim_base.TIM_CounterMode       = TIM_CounterMode_Up;
    tim_base.TIM_Period            = 65535U;  // 16位最大值
    tim_base.TIM_Prescaler         = 0U;      // 不分频
    tim_base.TIM_RepetitionCounter = 0U;

    TIM_ICStructInit(&tim_ic);
    tim_ic.TIM_ICFilter = 0xF;  // 最大数字滤波

    /* 4. 初始化左编码器（TIM3） */
    TIM_TimeBaseInit(HALL_ENC_LEFT_TIMER, &tim_base);
    tim_ic.TIM_Channel = TIM_Channel_1;
    TIM_ICInit(HALL_ENC_LEFT_TIMER, &tim_ic);
    tim_ic.TIM_Channel = TIM_Channel_2;
    TIM_ICInit(HALL_ENC_LEFT_TIMER, &tim_ic);
    TIM_EncoderInterfaceConfig(
        HALL_ENC_LEFT_TIMER,
        TIM_EncoderMode_TI12,
        TIM_ICPolarity_Rising,
        TIM_ICPolarity_Falling
    );
    TIM_Cmd(HALL_ENC_LEFT_TIMER, ENABLE);

    /* 5. 初始化右编码器（TIM4） */
    TIM_TimeBaseInit(HALL_ENC_RIGHT_TIMER, &tim_base);
    tim_ic.TIM_Channel = TIM_Channel_1;
    TIM_ICInit(HALL_ENC_RIGHT_TIMER, &tim_ic);
    tim_ic.TIM_Channel = TIM_Channel_2;
    TIM_ICInit(HALL_ENC_RIGHT_TIMER, &tim_ic);
    TIM_EncoderInterfaceConfig(
        HALL_ENC_RIGHT_TIMER,
        TIM_EncoderMode_TI12,
        TIM_ICPolarity_Rising,
        TIM_ICPolarity_Falling
    );
    TIM_Cmd(HALL_ENC_RIGHT_TIMER, ENABLE);
}

/**
 * @brief 获取指定霍尔编码器的增量脉冲数（自上次读取以来）
 * @param id: 编码器通道（ENCODER_LEFT 或 ENCODER_RIGHT）
 * @return int16_t: 增量值（单位：编码器脉冲数）
 *         - 正值：正向旋转
 *         - 负值：反向旋转
 *         - 零：无变化
 *
 * @details 
 *   采用“差值法”避免清零导致的脉冲丢失，并自动处理 16 位计数器溢出。
 *   首次调用返回自初始化以来的总增量。
 */
static uint16_t s_last_cnt[2] = {0};  // [0]: left, [1]: right

int16_t Hall_Encoder_Get(EncoderId_t id)
{
    TIM_TypeDef* tim = (id == ENCODER_LEFT) ? HALL_ENC_LEFT_TIMER : HALL_ENC_RIGHT_TIMER;
    uint16_t current = tim->CNT;
    uint16_t last = s_last_cnt[id - 1];
    s_last_cnt[id - 1] = current;
    return (int16_t)(current - last);
}

