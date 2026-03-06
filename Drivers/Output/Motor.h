#ifndef __MOTOR_H
#define __MOTOR_H

#include <stdint.h>

/**
 * @brief 电机标识符
 */
typedef enum {
    MOTOR_LEFT  = 0,
    MOTOR_RIGHT = 1,
    MOTOR_COUNT = 2
} MotorId_t;

/**
 * @brief 初始化电机驱动系统（PWM + 方向控制）
 */
void Motor_Init(void);

/**
 * @brief 设置指定电机的转速与方向
 * @param id: 电机标识符（ MOTOR_LEFT / MOTOR_RIGHT）
 * @param speed: 速度指令，范围 [-PWM_RESOLUTION, +PWM_RESOLUTION]
 *        - 正值：正转
 *        - 负值：反转
 *        - 零：刹车停止
 * @note 实际占空比 = |speed|，方向由 H 桥控制引脚决定
 */
void Motor_SetSpeed(MotorId_t id, int16_t speed);

#endif /* __MOTOR_H */
