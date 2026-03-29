// menu_callbacks.h
#ifndef MENU_CALLBACKS_H
#define MENU_CALLBACKS_H
#include "stm32f10x.h"                  // Device header

void Test_Callback_1(void);
void Calibration_Callback(void);
void Save_PID_Callback(void);          // 保存 PID 参数回调
void Reset_PID_Callback(void);         // 重置 PID 参数回调

#endif
