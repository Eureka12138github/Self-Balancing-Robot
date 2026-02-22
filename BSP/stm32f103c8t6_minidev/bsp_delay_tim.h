#ifndef __DELAY_H
#define __DELAY_H
#include "stm32f10x.h"                  // Device header
#include <stdint.h>

void Delay_TIM_Init(void);
void Delay_us(uint32_t us);
void Delay_ms(uint32_t ms);
void Delay_s(uint32_t s);


#endif
