#ifndef __LED_H
#define __LED_H

#include "stm32f10x.h"

/**
 ******************************************************************************
 * @file    led.h
 * @brief   LED 驱动头文件（基于 STM32 标准外设库）
 *
 * @note    【硬件接线说明】
 *          本驱动假设 LED 采用 **共阳极接法**：
 *            - LED 阳极接 VCC（3.3V）
 *            - LED 阴极通过限流电阻（如 220Ω~1kΩ）接到 MCU 的 GPIO 引脚
 *          → 因此：**GPIO 输出低电平（0）时 LED 亮，高电平（1）时 LED 灭**
 *
 *          如果你的电路是共阴极（LED 阴极接地），请交换 ON/OFF 宏定义！
 ******************************************************************************
 */

// ==============================================================================
// === 【用户配置区】—— 修改以下宏即可适配不同引脚或端口 =========================
// ==============================================================================

#define LED_GPIO_PORT        GPIOC          // LED 所在的 GPIO 端口（如 GPIOA, GPIOB...）
#define LED1_PIN             GPIO_Pin_13     // LED1 连接的引脚（如 GPIO_Pin_0 表示 PA0）
#define LED2_PIN             GPIO_Pin_1     // LED2 连接的引脚（如 GPIO_Pin_1 表示 PA1）

// 自动匹配 RCC 时钟使能位（必须与 LED_GPIO_PORT 一致）
#define LED_RCC_APB2Periph   RCC_APB2Periph_GPIOA

// ==============================================================================
// === 【LED 控制接口】—— 直接调用这些“函数”即可（实际为宏，无函数调用开销）===
// ==============================================================================

/**
 * @brief  点亮 LED1（输出低电平）
 * @note   仅适用于共阳极接法
 */
#define LED1_ON()    GPIO_ResetBits(LED_GPIO_PORT, LED1_PIN)

/**
 * @brief  熄灭 LED1（输出高电平）
 */
#define LED1_OFF()   GPIO_SetBits(LED_GPIO_PORT, LED1_PIN)

/**
 * @brief  反转 LED1 当前状态
 */
#define LED1_Turn()  do { \
    if (GPIO_ReadOutputDataBit(LED_GPIO_PORT, LED1_PIN)) \
        GPIO_ResetBits(LED_GPIO_PORT, LED1_PIN); \
    else \
        GPIO_SetBits(LED_GPIO_PORT, LED1_PIN); \
} while(0)


/**
 * @brief  点亮 LED2（输出低电平）
 */
#define LED2_ON()    GPIO_ResetBits(LED_GPIO_PORT, LED2_PIN)

/**
 * @brief  熄灭 LED2（输出高电平）
 */
#define LED2_OFF()   GPIO_SetBits(LED_GPIO_PORT, LED2_PIN)

/**
 * @brief  反转 LED2 当前状态
 */
#define LED2_Turn()  do { \
    if (GPIO_ReadOutputDataBit(LED_GPIO_PORT, LED2_PIN)) \
        GPIO_ResetBits(LED_GPIO_PORT, LED2_PIN); \
    else \
        GPIO_SetBits(LED_GPIO_PORT, LED2_PIN); \
} while(0)


// ==============================================================================
// === 【函数声明】===============================================================
// ==============================================================================

/**
 * @brief  初始化 LED 对应的 GPIO 为推挽输出模式（50MHz）
 * @note   调用后 LED 默认处于熄灭状态（高电平）
 */
void LED_Init(void);

#endif /* __LED_H */
