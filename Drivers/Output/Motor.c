#include "motor.h"
#include "PWM.h"

/**
 * @brief 电机初始化函数
 * @details 初始化PWM模块和电机方向控制引脚
 */
void Motor_Init(void)
{
    // 初始化 PWM（假设使用 TIMx_CHy，由 PWM.h 内部管理）
    PWM_Init();

    // 使能方向控制引脚所在 GPIO 时钟
    RCC_APB2PeriphClockCmd(MOTOR_RCC_APB2Periph, ENABLE);

    // 配置方向控制引脚为推挽输出
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = MOTOR_PIN_FORWARD | MOTOR_PIN_REVERSE;
    GPIO_Init(MOTOR_DIR_PORT, &GPIO_InitStructure);

    // 初始状态：停止（两引脚都低）
    GPIO_ResetBits(MOTOR_DIR_PORT, MOTOR_PIN_FORWARD | MOTOR_PIN_REVERSE);
}

/**
 * @brief 设置电机转速和方向
 * @param Speed: 正值→正转，负值→反转，0→停止；范围建议 -100 ~ +100
 */
void Motor_SetPWM(int8_t PWM)
{
    if (PWM > 0)
    {
        // 正转：Forward=1, Reverse=0
        GPIO_SetBits(MOTOR_DIR_PORT, MOTOR_PIN_FORWARD);
        GPIO_ResetBits(MOTOR_DIR_PORT, MOTOR_PIN_REVERSE);
        PWM_SetCompare1(PWM);      // 占空比 = Speed
    }
    else if (PWM < 0)
    {
        // 反转：Forward=0, Reverse=1
        GPIO_ResetBits(MOTOR_DIR_PORT, MOTOR_PIN_FORWARD);
        GPIO_SetBits(MOTOR_DIR_PORT, MOTOR_PIN_REVERSE);
        PWM_SetCompare1(-PWM);     // 取绝对值
    }
    else
    {
        // 停止：两引脚都为0（刹车模式）
        GPIO_ResetBits(MOTOR_DIR_PORT, MOTOR_PIN_FORWARD | MOTOR_PIN_REVERSE);
        PWM_SetCompare1(0);
    }
}
