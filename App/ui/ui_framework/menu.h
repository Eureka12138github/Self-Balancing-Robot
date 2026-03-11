/**
 * @file my_oled_menu.h
 * @brief 轻量级OLED菜单UI框架接口头文件
 * 
 * 本文件定义了OLED菜单系统的核心对外接口函数。
 * 提供了完整的菜单导航、显示控制和用户交互功能。
 * 采用模块化设计，便于集成到各种嵌入式项目中。
 * 
 * 主要功能：
 * - 菜单系统初始化和配置
 * - 按键事件处理和页面导航
 * - 垂直滚动和水平文本滚动
 * - 稳定的帧率控制机制
 * 
 * @author Eureka & Lingma
 * @date 2026-02-22
 */

#ifndef MY_OLED_MENU_H
#define MY_OLED_MENU_H

#include "menu_core_types.h"

#ifdef __cplusplus
extern "C" {
#endif

	
//#define USE_ENCODER_INPUT	//若使用编码器输入则取消此注释，引脚在 bsp_config 中修改
	
/*============================================================================
 *                          对外接口函数声明
 *============================================================================*/

/**
 * @brief 初始化OLED菜单系统
 * 
 * 配置并启动菜单UI框架，设置初始显示页面
 * 
 * @param page 指向初始页面的指针，不能为空
 * @note 必须在系统启动时调用一次，且应在OLED硬件初始化之后
 * @warning 若传入NULL指针，函数将显示错误信息并进入死循环
 */
void MyOLED_UI_Init(MyMenuPage* page);

/**
 * @brief 菜单系统主循环函数
 * 
 * UI系统的核心循环函数，应持续调用以维持菜单系统运行
 * 处理按键输入、更新显示状态并刷新OLED屏幕
 * 
 * @note 需要在main函数的while(1)循环中持续调用
 * @warning 调用频率影响UI响应速度，建议至少30FPS
 */
void MyOLED_UI_MainLoop(void);

#ifdef __cplusplus
}
#endif

#endif // MY_OLED_MENU_H

/*============================================================================
 *                          使用说明和调用示例
 *============================================================================*/

/*
 * ==================== 使用说明 ====================
 * 
 * 1. 系统初始化顺序：
 *    - 硬件初始化(OLED、按键、定时器等)
 *    - 调用MyOLED_UI_Init()初始化菜单系统
 *    - 在主循环中持续调用MyOLED_UI_MainLoop()
 * 
 * 2. 集成步骤：
 *    - 包含本头文件
 *    - 定义菜单数据(my_menu_data.c)
 *    - 按照示例配置main函数
 * 
 * 3. 注意事项：
 *    - 确保定时器中断正常工作(用于按键扫描和滚动动画)
 *    - OLED屏幕分辨率应为128x64像素
 *    - 按键需要配置相应的GPIO和中断
 * ================================================
 */

/*
 * ==================== 典型调用示例 ====================
 * 
 * // main.c - 系统主函数示例
 * #include "my_oled_menu.h"
 * #include "my_menu_data.h"
 * #include "bsp_oled.h"
 * #include "bsp_keys.h"
 * #include "bsp_delay.h"
 * 
 * int main(void) {
 *     // 1. 系统时钟和基本外设初始化
 *     SystemInit();
 *     Delay_Init();
 *     
 *     // 2. OLED显示初始化
 *     OLED_Init();
 *     
 *     // 3. 按键初始化
 *     Key_Init();
 *     
 *     // 4. 定时器初始化(用于按键扫描)
 *     Timer2_Init();
 *     
 *     // 5. 初始化菜单系统(传入主页面)
 *     MyOLED_UI_Init(&MainPage);
 *     
 *     // 6. 主循环 - 持续运行菜单系统
 *     while(1) {
 *         MyOLED_UI_MainLoop();
 *     }
 * }
 * 
 * ================================================
 */
