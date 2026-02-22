#ifndef __BSP_ALARM_H
#define __BSP_ALARM_H
#include "stm32f10x.h"                  // Device header
#define Buzzer_IO 	GPIO_Pin_7
#define Led_IO 		GPIO_Pin_13
#define Led 				(0)
#define Buzzer				(1)
#define Led_And_Buzzer		(2)
void Alarm_Init(void);
void Buzzer_Turn(void);
void Led_Turn(void);
void Alarm_Off_Auto(u8 Tpye);
	
#endif 


//以空行结尾
