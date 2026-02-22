#ifndef BSP_IWD_H
#define BSP_IWD_H
#include "stm32f10x.h"                  // Device header
#include "bsp_delay.h"
#include "storage.h"
#include "error_warning_log.h"
void MYIWD_Init(uint16_t MaxTime);
void Check_Reset_Way(void);


#endif
