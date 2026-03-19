/**
 * @file System_Init.c
 * @brief 系统初始化模块 - 集中管理所有硬件初始化
 * 
 * 设计原则:
 * - 统一管理所有硬件初始化
 * - main.c 只需调用 Initialize_System()
 * - 避免 main.c 直接依赖底层驱动
 */

#include "System_Init.h"
#include "system_config.h"     // 系统全局配置
#include "alarm.h"             // 警报模块
#include "iwdg.h"              // 看门狗
#include "menu_data.h"         // 菜单数据
#include "menu.h"              // UI 菜单
#include "led.h"               // LED 驱动
#include "hall_encoder.h"      // 霍尔编码器
#include "Motor.h"             // 电机驱动
#include "usart.h"             // 串口驱动
#include "MPU6050.h"           // MPU6050 驱动
#include "Delay.h"             // 延时函数
#include "BlueSerial.h"        // 蓝牙串口

/**
 * @brief  初始化系统。
 * @retval None
 */
void Initialize_System(void) {
    MyOLED_UI_Init(&MainPage);        				// MainPage 在 my_menu_data.h 中定义	
    Alarm_Init();									//警报初始化
		Usart1_Init(9600); 								//初始化调试串口
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);  //优先级分组配置   
		MYIWD_Init(2000);
		Motor_Init();
		Hall_Encoder_Init();
		MPU6050_Init();
		BlueSerial_Init();
	
		LED_Init();
		LED1_OFF();
		

}
