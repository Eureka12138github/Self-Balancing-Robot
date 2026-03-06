/**
 * @file rotary_encoder.c
 * @brief 旋转编码器驱动（正交解码，基于 EXTI 中断）
 *
 * 通过检测 A/B 两相信号的相位关系实现方向判别：
 *   - A 相下降沿时，若 B 为低 → 反转（计数减）
 *   - B 相下降沿时，若 A 为低 → 正转（计数加）
 *
 * ⚙️ 配置说明：
 *   - 引脚、端口、中断通道等参数在 rotary_encoder.h 中定义
 *   - 支持两种中断模式：
 *       • 独立中断（如 EXTI3/4，用于 Pin0~4）
 *       • 共用中断（如 EXTI9_5 / EXTI15_10，用于 Pin5~15）
 *
 * 🔧 使用步骤：
 *   1. 在 rotary_encoder.h 中配置引脚和 IRQ
 *   2. 在本文件中启用对应的中断服务函数（取消注释）
 *   3. 调用 Encoder_Init() 初始化，Encoder_Get() 获取增量值
 */

#include "rotary_encoder.h"
#include "bsp_config.h"

/** 编码器累计增量值（正转+，反转-） */
volatile int16_t Encoder_Count = 0;

// 滤波状态变量（用于抑制反向毛刺）

/**
 * @brief 初始化旋转编码器接口
 *
 * 配置 GPIO 为下拉输入，映射到 EXTI，设置下降沿触发中断，
 * 并使能 NVIC。需在 main() 中调用一次。
 */
void Encoder_Init(void)
{
    // 1. 使能 GPIO 和 AFIO 时钟
    RCC_APB2PeriphClockCmd(ENCODER_GPIO_CLK, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_AFIO, ENABLE);

    // 2. 配置 A/B 相引脚为下拉输入
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPD;
    GPIO_InitStructure.GPIO_Pin = ENCODER_PIN_A | ENCODER_PIN_B;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(ENCODER_GPIO_PORT, &GPIO_InitStructure);

    // 3. 将引脚映射到 EXTI 线
    GPIO_EXTILineConfig(ENCODER_EXTI_PORTSOURCE_A, ENCODER_PIN_SRC_A);
    GPIO_EXTILineConfig(ENCODER_EXTI_PORTSOURCE_B, ENCODER_PIN_SRC_B);

    // 4. 配置 EXTI 为下降沿触发中断
    EXTI_InitTypeDef EXTI_InitStructure;
    EXTI_InitStructure.EXTI_Line = ENCODER_EXTI_LINE;
    EXTI_InitStructure.EXTI_LineCmd = ENABLE;
    EXTI_InitStructure.EXTI_Mode = EXTI_Mode_Interrupt;
    EXTI_InitStructure.EXTI_Trigger = EXTI_Trigger_Falling;
    EXTI_Init(&EXTI_InitStructure);

    // 5. 配置 NVIC 中断优先级
    NVIC_InitTypeDef NVIC_InitStructure;
    
    // A 相中断
    NVIC_InitStructure.NVIC_IRQChannel = ENCODER_IRQ_A;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = ENCODER_EXTIA_PRIO;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = SUB_PRIO_UNUSED;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);

    // B 相中断
    NVIC_InitStructure.NVIC_IRQChannel = ENCODER_IRQ_B;
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = ENCODER_EXTIB_PRIO;
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = SUB_PRIO_UNUSED + 1;
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    NVIC_Init(&NVIC_InitStructure);
}

/**
 * @brief 获取并清零当前编码器增量值（带方向滤波）
 * @return int16_t 自上次调用以来的有效增量（通常为 +1 或 -1）
 *
 * 📌 滤波策略：
 *   - 仅当连续 ≥2 次同方向才输出
 *   - 返回值强制为 ±1（避免 UI 跳跃）
 */
static volatile int8_t last_direction = 0;
static volatile uint8_t direction_count = 0;

/**
 * @brief 获取编码器增量（带方向一致性滤波）
 * @return int16_t 过滤后的增量（通常为 -1, 0, +1；快速旋转可能为 ±2 等）
 * @problem “即使不反转方向也存在滞后”现象，在面对 中间毛刺（如 -1, -1, +1, -1, -1）时产生的“方向误判 + 状态重置”所导致的延迟响应。          
 */
int8_t Encoder_Get(void)
{
    // 1. 原子读取并清零原始计数
    __disable_irq();
    int16_t raw = Encoder_Count;
    Encoder_Count = 0;
    __enable_irq();

    if (raw == 0) {
        return 0;
    }

    // 2. 判断主方向
    int8_t current_dir = (raw > 0) ? 1 : -1;

    // 3. 方向一致性滤波
    if (current_dir == last_direction) {
        direction_count++;
    } else {
        last_direction = current_dir;
        direction_count = 1;
    }

    // 4. 只有连续 ≥2 次同方向才输出（可调）
    if (direction_count >= 2) {
        return raw > 0 ? 1 : -1;
    } else {
        return 0; // 丢弃孤立噪声脉冲
    }
}

// ----------------------------------------------------------------------------
// 中断服务函数（独立中断：适用于 EXTI0 / EXTI1）
// ----------------------------------------------------------------------------

void EXTI0_IRQHandler(void)
{
    if (EXTI_GetITStatus(ENCODER_A_EXTI_LINE) != RESET) {
        // A 相下降沿：若 B 为低 → 反转
        if (GPIO_ReadInputDataBit(ENCODER_GPIO_PORT, ENCODER_PIN_B) == Bit_RESET) {
            Encoder_Count--;
        }
        EXTI_ClearITPendingBit(ENCODER_A_EXTI_LINE);
    }
}

void EXTI1_IRQHandler(void)
{
    if (EXTI_GetITStatus(ENCODER_B_EXTI_LINE) != RESET) {
        // B 相下降沿：若 A 为低 → 正转
        if (GPIO_ReadInputDataBit(ENCODER_GPIO_PORT, ENCODER_PIN_A) == Bit_RESET) {
            Encoder_Count++;
        }
        EXTI_ClearITPendingBit(ENCODER_B_EXTI_LINE);
    }
}
