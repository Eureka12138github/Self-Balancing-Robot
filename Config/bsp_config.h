/**
 ******************************************************************************
 * @file    bsp_config.h
 * @author  Eureka
 * @brief   STM32F103C8T6 最小系统板外设硬件抽象层（BSP）配置
 *
 *          定义所有外设（OLED、PMS7003、ESP8266、XM7903）的引脚、时钟、
 *          串口、DMA 等底层连接参数。修改此文件即可适配不同硬件布局。
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
#define OLED_ENABLE_ASSERTIONS					/*!< 开发时打开断言，最终版本注释掉此宏定义 */
#define OLED_SCL_PIN        GPIO_Pin_8          /*!< I2C 时钟线 SCL 引脚 */
#define OLED_SDA_PIN        GPIO_Pin_9          /*!< I2C 数据线 SDA 引脚 */
#define OLED_GPIO_PORT      GPIOB               /*!< SCL/SDA 所在 GPIO 端口 */
#define OLED_GPIO_CLK       RCC_APB2Periph_GPIOB/*!< 对应 GPIO 时钟使能位 */
#define SCL_SDA_DELAY_US    0					/*!< 如有必要，可加入延时，避免超出I2C通信的最大速度 */

#define OLED_I2C_ADDR       0x78                /*!< OLED 模块 I2C 地址（7位左移后为 0x78） */
/** @} */


/* ============================================================================ */
/*                            通用I2C总线配置                                   */
/* ============================================================================ */
/** @defgroup I2C_Config 通用I2C引脚配置 */
/** @{ */

/* 从bsp_config.h继承I2C引脚配置 */
//#define I2C_SCL_PIN         GPIO_Pin_8        /*!< I2C时钟线SCL引脚 */
//#define I2C_SDA_PIN         GPIO_Pin_9        /*!< I2C数据线SDA引脚 */
//#define I2C_GPIO_PORT       GPIOB      /*!< SCL/SDA所在GPIO端口 */
//#define I2C_GPIO_CLK        RCC_APB2Periph_GPIOB       /*!< 对应GPIO时钟使能位 */

/** @} */


/* ============================================================================ */
/*                             按键输入配置                                     */
/* ============================================================================ */
/** @defgroup Keys_Config 按键输入配置 */
/** @{ */

/*---------------- 硬件引脚配置（修改此处调整按键引脚） ----------------*/
#define Key1_PIN        GPIO_Pin_6     // 按键1 -> PA6
#define Key2_PIN        GPIO_Pin_7     // 按键2 -> PA7
#define Key3_PIN        GPIO_Pin_0     // 按键3 -> PB0
#define Key4_PIN        GPIO_Pin_1     // 按键4 -> PB1

/* 按键状态读取宏（0=按下，1=释放） */
#define Read_Key1_State()   GPIO_ReadInputDataBit(GPIOA, Key1_PIN)
#define Read_Key2_State()   GPIO_ReadInputDataBit(GPIOA, Key2_PIN)
#define Read_Key3_State()   GPIO_ReadInputDataBit(GPIOB, Key3_PIN)
#define Read_Key4_State()   GPIO_ReadInputDataBit(GPIOB, Key4_PIN)

/*---------------- 时间参数配置（基于20ms系统时钟） -------------------*/
#define DEBOUNCE_TICKS      1       // 消抖时间20ms 
#define LONG_PRESS_TICKS    25      // 长按判定500ms
#define DOUBLE_CLICK_TICKS  10      // 双击间隔200ms
#define TRIPLE_CLICK_TICKS  10      // 三击间隔200ms 

/* 渐进长按阶段配置 */
#define PHASE1_END_TICKS    100     // 阶段1结束2000ms
#define PHASE2_END_TICKS    200     // 阶段2结束4000ms  
#define INTERVAL_PHASE1     10      // 阶段1触发间隔200ms
#define INTERVAL_PHASE2     5       // 阶段2触发间隔100ms
#define INTERVAL_PHASE3     1       // 阶段3触发间隔20ms

#define MAX_KEYS_NUM       4       // 最大按键数量

/** @} */



#endif /* BSP_CONFIG_H */
