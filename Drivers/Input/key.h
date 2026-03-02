#ifndef KEY_H
#define KEY_H
#include "stm32f10x.h"                  // Device header                 
/*--------------------- 状态与事件类型定义 --------------------------*/
typedef enum {
    KEY_STATE_IDLE,          // 空闲状态
    KEY_STATE_DEBOUNCE,      // 消抖检测
    KEY_STATE_PRESSED,       // 持续按压
    KEY_STATE_WAIT_DOUBLE,   // 等待双击
    KEY_STATE_WAIT_TRIPLE    // 等待三击
} KeyState;

typedef enum {
    KEY_EVENT_NONE,          // 无事件
    KEY_EVENT_CLICK,         // 单击
    KEY_EVENT_DOUBLE_CLICK,  // 双击
    KEY_EVENT_TRIPLE_CLICK,  // 三击
    KEY_EVENT_LONG_PRESS     // 长按
} KeyEventType;

void Key_Init(void); // 按键初始化
void Key_Scan(void);    // 按键状态扫描（需20ms周期调用）
void Key_GetEvent(KeyEventType* Key_Event, uint8_t Event_Num); // 获取按键事件


#endif
