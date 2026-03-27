#ifndef CONTROL_H_
#define CONTROL_H_

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 *                      控制函数声明
 *============================================================================*/

/** @brief 平衡控制模块初始化 */
void Balance_Init(void);

/** @brief 平衡控制主调度器 */
void Balance_Control_Scheduler(void);

/*============================================================================
 *                      监控数据结构（用于 UI 显示）
 *============================================================================*/

/** @brief 平衡车运行状态监控数据 */
typedef struct {
    float angle;           // 当前角度（度）
    float angleAcc;        // 加速度计角度（度）
    int16_t left_pwm;      // 左轮 PWM
    int16_t right_pwm;     // 右轮 PWM
    int16_t dif_pwm;       // 差速 PWM
    float ave_speed;       // 平均速度（rps）
    float dif_speed;       // 速度差（rps）
} BalanceMonitor_t;

/**
 * @brief 获取当前监控数据快照
 * @param monitor 指向监控结构体的指针
 */
void Balance_GetMonitorData(BalanceMonitor_t* monitor);

#endif
