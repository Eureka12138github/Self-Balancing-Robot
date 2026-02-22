#include "bsp_input.h"
#include "bsp_config.h"
/**
 * @brief 初始化按键
 *
 * 该函数初始化按键所需的 GPIO 端口和时钟。具体步骤包括：
 * 1. 使能 GPIOA 和 GPIOB 的时钟。
 * 2. 配置 GPIOA 和 GPIOB 的端口模式为上拉输入模式。
 * 3. 设置端口速度为 50 MHz（在输入模式下，速度配置实际上不起作用，但为了完整性仍进行配置）。
 *
 * @note
 * - `RCC_APB2PeriphClockCmd` 用于使能 GPIO 时钟。
 * - `GPIO_InitTypeDef` 结构体用于配置 GPIO 端口。
 * - `GPIO_Mode_IPU` 表示上拉输入模式。
 * - `Key1_PIN`, `Key2_PIN`, `Key3_PIN`, `Key4_PIN` 是按键对应的 GPIO 引脚。
 */
void Key_Init(void)
{
    // 使能 GPIOA 和 GPIOB 的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure; // 定义结构体变量

    // 配置 GPIOA 的按键引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入模式
    GPIO_InitStructure.GPIO_Pin = Key1_PIN | Key2_PIN; // 选择需要配置的端口
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 在输入模式下，这里其实不用配置
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 GPIOB 的按键引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入模式
    GPIO_InitStructure.GPIO_Pin = Key3_PIN | Key4_PIN; // 选择需要配置的端口
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 在输入模式下，这里其实不用配置
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}
