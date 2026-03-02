#include "System_Init.h"
#include "system_config.h"
#include "error_warning_log.h"
#include "alarm.h"
#include "timer.h"
#include "iwdg.h"
#include "menu_data.h"
#include "menu.h"
#include "led.h"
/**
 * @brief  初始化系统。
 * @retval None
 */
void Initialize_System(void) {
    MyOLED_UI_Init(&MainPage);        				// MainPage 在 my_menu_data.h 中定义	
    Alarm_Init();									//警报初始化
	Usart3_Init(9600); 								//初始化调试串口
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);  //优先级分组配置   
    MYIWD_Init(2000);
	RP_Init();
//	LED_Init();
//	LED1_ON();
		

}
