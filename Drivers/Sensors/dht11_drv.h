#ifndef __DHT11_DRV_H
#define __DHT11_DRV_H
#include "stm32f10x.h"
#include <stdbool.h> 
// Device header
#define Pin_Select GPIO_Pin_12
#define DHT11_High GPIO_SetBits(GPIOB, Pin_Select)
#define DHT11_Low GPIO_ResetBits(GPIOB, Pin_Select)
#define DHT11_DQ GPIO_ReadInputDataBit(GPIOB, Pin_Select)
bool DHT11_Init(void);
bool DHT11_Read_Data(u16 *temp,u16 *humi);
bool DHT11_Check(void);
void DHT11_Rst(void);

#endif 
