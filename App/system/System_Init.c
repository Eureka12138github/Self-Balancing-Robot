#include "System_Init.h"
#include "OLED.h"
#include "system_config.h"
#include "error_warning_log.h"
#include "bsp_alarm.h"
#include "bsp_timer.h"
#include "bsp_iwdg.h"
#include "my_menu_data.h"
#include "my_oled_menu.h"

/**
 * @brief  初始化系统。
 * @retval None
 */
void Initialize_System(void) {
    MyOLED_UI_Init(&MainPage);        				// MainPage 在 my_menu_data.h 中定义	
    Alarm_Init();									//警报初始化
	Usart3_Init(9600); 								//初始化调试串口
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);  //优先级分组配置                 
    MYIWD_Init(2000);  

}
