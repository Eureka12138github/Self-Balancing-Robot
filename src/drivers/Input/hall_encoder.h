// hall_encoder.h
#ifndef __HALL_ENCODER_H
#define __HALL_ENCODER_H

#include <stdint.h>

/**
 * @brief 编码器通道标识符
 */
typedef enum {
    ENCODER_LEFT  = 1,  ///< 左轮编码器（连接 TIM3: PA6/PA7）
    ENCODER_RIGHT = 2   ///< 右轮编码器（连接 TIM4: PB6/PB7）
} EncoderId_t;

void Hall_Encoder_Init(void);
int16_t Hall_Encoder_Get(EncoderId_t id);

#endif /* __HALL_ENCODER_H */
