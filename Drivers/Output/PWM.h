#ifndef __PWM_H
#define __PWM_H

#include <stdint.h>

// PWM 波形参数
#define PWM_FREQ_HZ             16000                    // 目标频率：2 kHz
#define PWM_RESOLUTION          100                     // 分辨率（ARR + 1），即 1% 步进

// =============================================================================
// 函数声明
// =============================================================================

void PWM_Init(void);
void PWM_SetCompare1(uint16_t Compare);  // 设置占空比，范围：0 ~ PWM_RESOLUTION
void PWM_SetCompare2(uint16_t Compare);

#endif /* __PWM_H */
