#ifndef DHT11_H
#define DHT11_H
#include "stm32f10x.h"
#include <stdbool.h> 
// Device header

//#define DHT11_GPIO_PORT      GPIOB              
//#define DHT11_GPIO_CLK       RCC_APB2Periph_GPIOB
//#define Pin_Select 			 GPIO_Pin_12
//#define DHT11_High() 		 GPIO_SetBits(DHT11_GPIO_PORT, Pin_Select)
//#define DHT11_Low()  	     GPIO_ResetBits(DHT11_GPIO_PORT, Pin_Select)
//#define DHT11_DQ 			 GPIO_ReadInputDataBit(DHT11_GPIO_PORT, Pin_Select)
bool DHT11_Init(void);
bool DHT11_Read_Data(u16 *temp,u16 *humi);
bool DHT11_Check(void);
void DHT11_Rst(void);

#endif 
