#ifndef __MOTOR_H
#define __MOTOR_H

#include "stm32f10x.h"

// =============================================================================
// 【用户配置区】—— 修改以下宏即可适配不同引脚或端口
// =============================================================================

// 方向控制引脚配置（必须在同一 GPIO 端口）
#define MOTOR_DIR_PORT          GPIOB
#define MOTOR_PIN_FORWARD       GPIO_Pin_12   // 正转控制引脚
#define MOTOR_PIN_REVERSE       GPIO_Pin_13   // 反转控制引脚
#define MOTOR_RCC_APB2Periph    RCC_APB2Periph_GPIOB


// =============================================================================
// 函数声明
// =============================================================================

void Motor_Init(void);
void Motor_SetPWM(int8_t PWM);

#endif /* __MOTOR_H */
