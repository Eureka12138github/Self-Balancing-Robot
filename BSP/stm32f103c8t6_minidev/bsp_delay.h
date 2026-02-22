#ifndef BSP_DELAY_H
#define BSP_DELAY_H
#include "stm32f10x.h"                  // Device header
#include <stdint.h>

/**
 * @brief  初始化延时系统（必须在 main 开头调用一次）
 */
void Delay_Init(void);

/**
 * @brief  微秒级阻塞延时
 * @param  us: 延时时间（微秒）
 */
void Delay_us(uint32_t us);

/**
 * @brief  毫秒级阻塞延时
 * @param  ms: 延时时间（毫秒）
 */
void Delay_ms(uint32_t ms);

/**
 * @brief  秒级阻塞延时
 * @param  s: 延时时间（秒）
 */
void Delay_s(uint32_t s);

/**
 * @brief  获取系统运行时间（ms）
 * @retval 自系统启动以来经过的毫秒数
 */
uint32_t SysTick_Get(void);

#endif 
