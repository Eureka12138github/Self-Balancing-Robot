#ifndef __MICRO_TIMER_H
#define __MICRO_TIMER_H
#include <stdint.h>
void MicroTimer_Init(void);
uint32_t Calculate_Elapsed_Time(uint32_t start, uint32_t end);
uint32_t Get_Micros(void);

#endif
