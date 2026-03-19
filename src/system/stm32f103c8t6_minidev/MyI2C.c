/**
 * @file my_i2c.c
 * @brief 软件模拟 I2C 主机驱动（Bit-banging）
 *
 * 实现标准 I2C 协议的起始/停止、字节收发、ACK/NACK 控制。
 * 使用 GPIO 模拟 SCL/SDA 时序，适用于无硬件 I2C 或引脚受限场景。
 *
 * 引脚可通过宏灵活配置，支持快速移植。
 *
 * @note 本实现假设系统主频 ≥ 8MHz，Delay_us(1~5) 可确保时序满足常见器件要求（如 OLED、MPU6050）
 * @warning 所有 I2C 操作必须在中断关闭或高优先级下执行，避免时序被打断
 */
#include "MyI2C.h"
#include "stm32f10x.h"
#include "Delay.h"

/* =============================================================================
 *                          引脚配置
 * ============================================================================= */
#define I2C_SCL_PORT        GPIOB
#define I2C_SCL_PIN         GPIO_Pin_10

#define I2C_SDA_PORT        GPIOB
#define I2C_SDA_PIN         GPIO_Pin_11

#define I2C_DELAY_US        (0U)   /*!< 时序微调延时，单位：微秒（设为 0 可关闭） */

/* =============================================================================
 *                          底层引脚操作封装
 * ============================================================================= */

/**
 * @brief 设置 SCL 引脚电平
 * @param BitValue: 0=低电平, 非0=高电平
 */
static void MyI2C_W_SCL(uint8_t BitValue)
{
    GPIO_WriteBit(I2C_SCL_PORT, I2C_SCL_PIN, (BitAction)BitValue);
#if (I2C_DELAY_US > 0)
    Delay_us(I2C_DELAY_US);
#endif
}

/**
 * @brief 设置 SDA 引脚电平
 * @param BitValue: 0=低电平, 非0=高电平
 */
static void MyI2C_W_SDA(uint8_t BitValue)
{
    GPIO_WriteBit(I2C_SDA_PORT, I2C_SDA_PIN, (BitAction)BitValue);
#if (I2C_DELAY_US > 0)
    Delay_us(I2C_DELAY_US);
#endif
}

/**
 * @brief 读取 SDA 引脚输入电平
 * @return 0 或 1
 */
static uint8_t MyI2C_R_SDA(void)
{
    uint8_t val = GPIO_ReadInputDataBit(I2C_SDA_PORT, I2C_SDA_PIN);
#if (I2C_DELAY_US > 0)
    Delay_us(I2C_DELAY_US);
#endif
    return val;
}

/* =============================================================================
 *                          I2C 协议层实现
 * ============================================================================= */

/**
 * @brief 初始化 I2C 软件引脚
 *
 * 将 SCL/SDA 配置为开漏输出模式，并释放总线（置高）。
 * @note 开漏模式允许从机拉低信号，符合 I2C 总线规范
 */
void MyI2C_Init(void)
{
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_OD;  // 开漏输出
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin   = I2C_SCL_PIN | I2C_SDA_PIN;
    GPIO_Init(I2C_SCL_PORT, &GPIO_InitStructure);

    // 释放总线：SCL=1, SDA=1 → 空闲状态
    MyI2C_W_SCL(1);
    MyI2C_W_SDA(1);
}

/**
 * @brief 发送 I2C 起始条件（START）
 *
 * 时序：SCL=1 时，SDA 由高→低
 * @note 支持重复起始（Repeated START）
 */
void MyI2C_Start(void)
{
    MyI2C_W_SDA(1);  // 确保 SDA 初始为高（兼容重复起始）
    MyI2C_W_SCL(1);
    MyI2C_W_SDA(0);  // SDA 下降沿
    MyI2C_W_SCL(0);  // 拉低 SCL，准备发送数据
}

/**
 * @brief 发送 I2C 停止条件（STOP）
 *
 * 时序：SCL=1 时，SDA 由低→高
 */
void MyI2C_Stop(void)
{
    MyI2C_W_SDA(0);
    MyI2C_W_SCL(1);
    MyI2C_W_SDA(1);  // SDA 上升沿
}

/**
 * @brief 主机发送一个字节（MSB 先传）
 * @param Byte: 待发送的 8 位数据
 */
void MyI2C_SendByte(uint8_t Byte)
{
    for (uint8_t i = 0; i < 8; i++) {
        MyI2C_W_SDA((Byte >> (7 - i)) & 0x01);  // 发送高位先
        MyI2C_W_SCL(1);
        MyI2C_W_SCL(0);
    }
}

/**
 * @brief 主机接收一个字节（MSB 先收）
 * @return 接收到的 8 位数据
 */
uint8_t MyI2C_ReceiveByte(void)
{
    uint8_t byte = 0;
    MyI2C_W_SDA(1);  // 主机释放 SDA（切换为输入）

    for (uint8_t i = 0; i < 8; i++) {
        MyI2C_W_SCL(1);
        if (MyI2C_R_SDA()) {
            byte |= (0x80 >> i);
        }
        MyI2C_W_SCL(0);
    }
    return byte;
}

/**
 * @brief 主机发送 ACK/NACK
 * @param AckBit: 0=ACK（应答），1=NACK（非应答）
 */
void MyI2C_SendAck(uint8_t AckBit)
{
    MyI2C_W_SDA(AckBit);  // 0=ACK, 1=NACK
    MyI2C_W_SCL(1);
    MyI2C_W_SCL(0);
}

/**
 * @brief 主机接收从机的 ACK/NACK
 * @return 0=ACK，1=NACK
 */
uint8_t MyI2C_ReceiveAck(void)
{
    uint8_t ack;
    MyI2C_W_SDA(1);  // 主机释放 SDA（高阻态）
    MyI2C_W_SCL(1);
    ack = MyI2C_R_SDA();
    MyI2C_W_SCL(0);
    return ack;
}
