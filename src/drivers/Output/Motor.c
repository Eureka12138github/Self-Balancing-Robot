#include "motor.h"
#include "PWM.h"          // 确保 PWM_RESOLUTION 在此可见
#include "stm32f10x.h"

// =============================================================================
// 【用户配置区】
// =============================================================================
#define MOTOR_DIR_PORT          GPIOB
#define MOTOR_RCC_APB2Periph    RCC_APB2Periph_GPIOB

#define MOTOR_LEFT_FWD_PIN      GPIO_Pin_12
#define MOTOR_LEFT_REV_PIN      GPIO_Pin_13
#define MOTOR_RIGHT_FWD_PIN     GPIO_Pin_14
#define MOTOR_RIGHT_REV_PIN     GPIO_Pin_15

// 定义引脚配置结构体类型（命名结构体）
struct motor_pin_config {
    uint16_t fwd_pin;
    uint16_t rev_pin;
};

// 引脚配置表
static const struct motor_pin_config g_motor_dir_pins[MOTOR_COUNT] = {
    { MOTOR_LEFT_FWD_PIN,  MOTOR_LEFT_REV_PIN  },
    { MOTOR_RIGHT_FWD_PIN, MOTOR_RIGHT_REV_PIN }
};


/**
 * @brief 内部：设置单个电机的方向与 PWM
 */
static void motor_set_direction_and_pwm(MotorId_t id, uint16_t abs_duty, uint8_t forward)
{
    const struct motor_pin_config *cfg = &g_motor_dir_pins[id];  // 显式类型
    if (forward) {
        GPIO_SetBits(MOTOR_DIR_PORT, cfg->fwd_pin);
        GPIO_ResetBits(MOTOR_DIR_PORT, cfg->rev_pin);
    } else {
        GPIO_ResetBits(MOTOR_DIR_PORT, cfg->fwd_pin);
        GPIO_SetBits(MOTOR_DIR_PORT, cfg->rev_pin);
    }

    if (id == MOTOR_LEFT) {
        PWM_SetCompare1(abs_duty);
    } else {
        PWM_SetCompare2(abs_duty);
    }
}

/**
 * @brief 电机初始化
 */
void Motor_Init(void)
{
    PWM_Init();

    RCC_APB2PeriphClockCmd(MOTOR_RCC_APB2Periph, ENABLE);

    GPIO_InitTypeDef gpio_init;
    gpio_init.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio_init.GPIO_Speed = GPIO_Speed_50MHz;
    gpio_init.GPIO_Pin = 
        MOTOR_LEFT_FWD_PIN | MOTOR_LEFT_REV_PIN |
        MOTOR_RIGHT_FWD_PIN | MOTOR_RIGHT_REV_PIN;
    GPIO_Init(MOTOR_DIR_PORT, &gpio_init);

    // 初始刹车
    GPIO_ResetBits(MOTOR_DIR_PORT, gpio_init.GPIO_Pin);
}

/**
 * @brief 设置电机速度
 */
void Motor_SetSpeed(MotorId_t id, int16_t speed)
{
    // 限幅：使用 PWM_RESOLUTION
    if (speed > (int16_t)PWM_RESOLUTION) {
        speed = (int16_t)PWM_RESOLUTION;
    } else if (speed < -(int16_t)PWM_RESOLUTION) {
        speed = -(int16_t)PWM_RESOLUTION;
    }

    if (speed > 0) {
        motor_set_direction_and_pwm(id, (uint16_t)speed, 1);
    } else if (speed < 0) {
        motor_set_direction_and_pwm(id, (uint16_t)(-speed), 0);
    } else {
        // 刹车
        const struct motor_pin_config *cfg = &g_motor_dir_pins[id];
        GPIO_ResetBits(MOTOR_DIR_PORT, cfg->fwd_pin | cfg->rev_pin);
        if (id == MOTOR_LEFT) {
            PWM_SetCompare1(0);
        } else {
            PWM_SetCompare2(0);
        }
    }
}
