/**
 * @file bsp_oled.h
 * @brief OLED显示屏硬件抽象层头文件
 * 
 * 定义了OLED显示屏操作所需的基本函数接口
 * 基于通用I2C驱动实现，支持多种OLED型号
 * 
 * @author Eureka & Lingma
 * @date 2026-02-22
 */

#ifndef BSP_OLED_H
#define BSP_OLED_H

#include "stm32f10x.h"
#include "system_config.h"  // 添加通用I2C头文件
#include <stdbool.h>
	
#ifdef OLED_ENABLE_ASSERTIONS
    void my_assert_handler(const char* expr, const char* file, int line);
    #define OLED_ASSERT(expr) do { \
        if (!(expr)) my_assert_handler(#expr, __FILE__, __LINE__); \
    } while(0)
#else
    #define OLED_ASSERT(expr) ((void)0)
#endif	
	

/*============================================================================
 *                          对外接口函数声明
 *============================================================================*/


/* I2C通信函数 */
void OLED_WriteCommand(uint8_t Command);
void OLED_WriteData(uint8_t *Data, uint8_t Count);

/* OLED 初始化函数 */

void OLED_Init(void);



#endif
