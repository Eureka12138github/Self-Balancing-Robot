/**
 ******************************************************************************
 * @file    bsp_config.h
 * @author  Eureka
 * @brief   STM32F103C8T6 最小系统板硬件配置中心
 *
 *          定义所有外设（OLED、DHT11、按键等）的引脚、时钟、
 *          中断优先级等底层参数。修改此文件即可适配不同硬件布局。
 *          应用层应通过驱动接口访问硬件，避免直接依赖本文件细节。
 ******************************************************************************
 */

#ifndef BSP_CONFIG_H
#define BSP_CONFIG_H

/* === 标准库依赖 === */
#include "stm32f10x.h"

/* ============================================================================ */
/*                            OLED 显示屏配置                                   */
/* ============================================================================ */
/** @defgroup OLED_Config OLED 引脚与通信参数 */
/** @{ */
#define OLED_SCL_PIN        GPIO_Pin_8          /*!< I2C 时钟线 SCL 引脚 (PB8) */
#define OLED_SDA_PIN        GPIO_Pin_9          /*!< I2C 数据线 SDA 引脚 (PB9) */
#define OLED_GPIO_PORT      GPIOB               /*!< SCL/SDA 所属 GPIO 端口 */
#define OLED_GPIO_CLK       RCC_APB2Periph_GPIOB/*!< 对应 GPIO 时钟使能位 */

#define SCL_SDA_DELAY_US    (0U)                /*!< 软件 I2C 延时（μs），0 表示无延时 */
#define OLED_I2C_ADDR       (0x78U)             /*!< OLED 模块 I2C 地址（7位左移后为 0x78） */
/** @} */


/* ============================================================================ */
/*                            DHT11 温湿度传感器配置                            */
/* ============================================================================ */
/** @defgroup DHT11_Config DHT11 引脚与操作宏 */
/** @{ */
#define DHT11_GPIO_PORT     GPIOB
#define DHT11_GPIO_CLK      RCC_APB2Periph_GPIOB
#define DHT11_PIN           GPIO_Pin_14         /*!< DHT11 数据引脚 (PB14) */

#define DHT11_HIGH()        GPIO_SetBits(DHT11_GPIO_PORT, DHT11_PIN)
#define DHT11_LOW()         GPIO_ResetBits(DHT11_GPIO_PORT, DHT11_PIN)
#define DHT11_READ()        GPIO_ReadInputDataBit(DHT11_GPIO_PORT, DHT11_PIN)
/** @} */


/* ============================================================================ */
/*                             按键输入配置                                     */
/* ============================================================================ */
/** @defgroup Keys_Config 按键硬件与行为参数 */
/** @{ */

/* ---------------- 硬件引脚定义 ---------------- */
#define KEY1_PIN        GPIO_Pin_0      /*!< 按键 1 连接至 PB0 */
#define KEY2_PIN        GPIO_Pin_11      /*!< 按键 2 连接至 PA11 */
#define KEY3_PIN        GPIO_Pin_5     /*!< 按键 3 连接至 PA5 */
#define KEY4_PIN        GPIO_Pin_4     /*!< 按键 4 连接至 PA4 */

/* ---------------- 按键状态读取宏 ---------------- */
#define READ_KEY1_STATE()   GPIO_ReadInputDataBit(GPIOB, KEY1_PIN)
#define READ_KEY2_STATE()   GPIO_ReadInputDataBit(GPIOA, KEY2_PIN)
#define READ_KEY3_STATE()   GPIO_ReadInputDataBit(GPIOA, KEY3_PIN)
#define READ_KEY4_STATE()   GPIO_ReadInputDataBit(GPIOA, KEY4_PIN)

/* ---------------- 时间参数（基于 20ms/tick 的定时器中断） ---------------- */
#define DEBOUNCE_TICKS      (1U)    /*!< 消抖时间：20 ms */
#define LONG_PRESS_TICKS    (25U)   /*!< 长按判定：500 ms */
#define DOUBLE_CLICK_TICKS  (10U)   /*!< 双击最大间隔：200 ms */
#define TRIPLE_CLICK_TICKS  (10U)   /*!< 三击最大间隔：200 ms */

/* 渐进式长按阶段配置 */
#define PHASE1_END_TICKS    (100U)  /*!< 阶段1结束：2000 ms */
#define PHASE2_END_TICKS    (200U)  /*!< 阶段2结束：4000 ms */
#define INTERVAL_PHASE1     (6U)   /*!< 阶段1触发间隔：120 ms */
#define INTERVAL_PHASE2     (3U)    /*!< 阶段2触发间隔：60 ms */
#define INTERVAL_PHASE3     (1U)    /*!< 阶段3触发间隔：20 ms */

#define MAX_KEYS_NUM        (4U)    /*!< 支持的最大按键数量 */
/** @} */

/* ============================================================================ */
/*                             旋转编码器输入配置                               */
/* ============================================================================ */
// ⚠️ 注意：旋转编码器功能已移除，以下配置保留供参考
#if 0  // 禁用旋转编码器配置

/* ---------------- 换引脚时需修改以下项(注意要确保引脚后续不会被其他初始覆盖占用) ---------------- */
#define ENCODER_GPIO_PORT           GPIOB               /*!< 编码器端口（如 GPIOB） */
#define ENCODER_GPIO_CLK            RCC_APB2Periph_GPIOB/*!< 对应的 GPIO 时钟 */
#define ENCODER_PIN_A               GPIO_Pin_0          /*!< A 相连接的引脚 */
#define ENCODER_PIN_B               GPIO_Pin_1         /*!< B 相连接的引脚 */
#define ENCODER_PIN_SRC_A           GPIO_PinSource0     /*!< A 相 EXTI 映射源（必须与引脚号一致） */
#define ENCODER_PIN_SRC_B           GPIO_PinSource1     /*!< B 相 EXTI 映射源（必须与引脚号一致） */
#define ENCODER_EXTI_PORTSOURCE_A   GPIO_PortSourceGPIOB/*!< A 相端口 EXTI 映射源 */
#define ENCODER_EXTI_PORTSOURCE_B   GPIO_PortSourceGPIOB/*!< B 相端口 EXTI 映射源 */
#define ENCODER_IRQ_A               EXTI0_IRQn        /*!< A 相对应的中断通道 */
#define ENCODER_IRQ_B               EXTI1_IRQn        /*!< B 相对应的中断通道 */
// ⚠️ 同时需在 rotary_encoder.c 文件中修改中断服务函数名称！

/* ---------------- 以下无需修改 ---------------- */
#define ENCODER_EXTI_LINE           (ENCODER_PIN_A | ENCODER_PIN_B) /*!< 使用的 EXTI 线 */
#define ENCODER_A_EXTI_LINE         ((uint32_t)(ENCODER_PIN_A))     /*!< A 相 EXTI 线（用于状态查询） */
#define ENCODER_B_EXTI_LINE         ((uint32_t)(ENCODER_PIN_B))     /*!< B 相 EXTI 线（用于状态查询） */

// ⚠️ 注意：由于 KEY1 已从 PB0 改为 PB12，以下中断优先级配置需要调整
// PB0 (EXTI0) -> 编码器 A 相，使用 PRE_PRIO_5
// PB1 (EXTI1) -> 编码器 B 相，使用 PRE_PRIO_6
// PB12 (EXTI12) -> KEY1，需使用 EXTI15_10 中断线

// #define ENCODER_EXTIA_PRIO       PRE_PRIO_5  /*!< 编码器通道 0 外部中断 */
// #define ENCODER_EXTIB_PRIO       PRE_PRIO_6  /*!< 编码器通道 1 外部中断 */
// （中断优先级已在系统配置中统一管理）

/* ⚠️ 注意：务必在 .c 文件中实现与所选 IRQ 匹配的中断服务函数，
 *         例如使用 EXTI9_5_IRQn 时，需实现 EXTI9_5_IRQHandler()
 */
#endif  // 禁用旋转编码器配置



/* ============================================================================ */
/*                            中断优先级配置                                    */
/* ============================================================================ */
/**
 * @defgroup ISR_Priority_Config NVIC 中断优先级（使用 Priority Group 4）
 * @{
 * @par 配置说明：
 * - 抢占优先级范围：0（最高）~ 15（最低）
 * - 子优先级无效（固定为 0）
 * - 关键外设（如编码器）分配较高优先级
 * - 调试/非关键外设使用较低优先级
 */
#define SUB_PRIO_UNUSED    (0U)

/* 抢占优先级常量 */
#define PRE_PRIO_0      (0U)
#define PRE_PRIO_1      (1U)
#define PRE_PRIO_2      (2U)
#define PRE_PRIO_3      (3U)
#define PRE_PRIO_4      (4U)
#define PRE_PRIO_5      (5U)
#define PRE_PRIO_6      (6U)
#define PRE_PRIO_7      (7U)
#define PRE_PRIO_8      (8U)
#define PRE_PRIO_9      (9U)
#define PRE_PRIO_10     (10U)
#define PRE_PRIO_11     (11U)
#define PRE_PRIO_12     (12U)
#define PRE_PRIO_13     (13U)
#define PRE_PRIO_14     (14U)
#define PRE_PRIO_15     (15U)

/* 功能语义化优先级别 */
// #define ENCODER_EXTIA_PRIO          PRE_PRIO_5  /*!< 编码器通道 0 外部中断（已删除） */
// #define ENCODER_EXTIB_PRIO          PRE_PRIO_6  /*!< 编码器通道 1 外部中断（已删除） */
#define UART_DEBUG_PRIO             PRE_PRIO_7  /*!< 调试串口（如 USART3） */
#define GENERAL_TASK_HANDLER_PRIO   PRE_PRIO_8  /*!< 通用任务调度定时器（如 TIM2） */

/* 具体外设中断优先级映射 */
/* USART */
#define USART1_PRIO     UART_DEBUG_PRIO
#define USART2_PRIO     PRE_PRIO_15
#define USART3_PRIO     PRE_PRIO_15

/* TIM */
#define TIMER1_PRIO     PRE_PRIO_3
#define TIMER2_PRIO     PRE_PRIO_15
#define TIMER3_PRIO     PRE_PRIO_15
#define TIMER4_PRIO     PRE_PRIO_15

/* EXTI */
#define EXTI0_PRIO      PRE_PRIO_5   /*!< PB0: 编码器 A 相 */
#define EXTI1_PRIO      PRE_PRIO_6   /*!< PB1: 编码器 B 相 */
#define EXTI2_PRIO      PRE_PRIO_15
#define EXTI3_PRIO      PRE_PRIO_15
#define EXTI4_PRIO      PRE_PRIO_15
#define EXTI9_5_PRIO    PRE_PRIO_15
#define EXTI15_10_PRIO  PRE_PRIO_15  /*!< PB12: KEY1 按键 */
/** @} */


#endif /* BSP_CONFIG_H */
