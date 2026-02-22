/**
 * @file bsp_i2c.c
 * @brief 通用软件模拟I2C总线驱动实现（支持多实例配置）
 * 
 * 本文件实现了基于GPIO模拟的I2C通信协议，支持运行时引脚配置
 * 可用于连接各类I2C设备如OLED显示屏、EEPROM、传感器等
 * 支持多实例管理，每个实例可配置不同的GPIO引脚
 * 
 * @author Eureka & Lingma
 * @date 2026-02-22
 */

#include "bsp_i2c.h"  
#include "bsp_config.h"
#include "stm32f10x.h"
#include "bsp_delay.h"
#include <stddef.h>


/*============================================================================
 *                          全局实例管理
 *============================================================================*/

/** @brief 最大支持的I2C实例数量 */
#define I2C_MAX_INSTANCES    4

/** @brief I2C实例配置数组 */
static I2C_PortConfig i2c_instances[I2C_MAX_INSTANCES] = {0};

/** @brief 实例计数器 */
static uint8_t instance_count = 0;

/*============================================================================
 *                          底层GPIO操作函数（带参数版本）
 *============================================================================*/

/**
 * @brief 写入指定GPIO的SCL电平状态
 * 
 * 控制指定GPIO端口的SCL引脚电平，是I2C通信的基础操作
 * 
 * @param gpio_port GPIO端口基地址
 * @param scl_pin SCL引脚号
 * @param bit_value 要写入的电平值：0=低电平，1=高电平
 * @note 此函数直接操作GPIO引脚，是I2C协议的物理层实现
 */
static void I2C_GPIO_WriteSCL_Param(GPIO_TypeDef* gpio_port, uint16_t scl_pin, uint8_t bit_value)
{
    GPIO_WriteBit(gpio_port, scl_pin, (BitAction)bit_value);
}

/**
 * @brief 写入指定GPIO的SDA电平状态
 * 
 * 控制指定GPIO端口的SDA引脚电平，用于传输数据和控制信号
 * 
 * @param gpio_port GPIO端口基地址
 * @param sda_pin SDA引脚号
 * @param bit_value 要写入的电平值：0=低电平，1=高电平
 * @note 此函数直接操作GPIO引脚，是I2C协议的物理层实现
 */
static void I2C_GPIO_WriteSDA_Param(GPIO_TypeDef* gpio_port, uint16_t sda_pin, uint8_t bit_value)
{
    GPIO_WriteBit(gpio_port, sda_pin, (BitAction)bit_value);
}

/**
 * @brief 带延时的SCL操作
 * 
 * 在操作SCL后添加可配置的延时，用于控制I2C通信速度
 * 
 * @param config I2C配置结构体指针
 * @param bit_value 要写入的电平值
 */
static void I2C_DelayedWriteSCL(const I2C_PortConfig* config, uint8_t bit_value)
{
    I2C_GPIO_WriteSCL_Param(config->gpio_port, config->scl_pin, bit_value);
    if (config->delay_us > 0) {
        Delay_us(config->delay_us);
    }
}

/**
 * @brief 带延时的SDA操作
 * 
 * 在操作SDA后添加可配置的延时，用于控制I2C通信速度
 * 
 * @param config I2C配置结构体指针
 * @param bit_value 要写入的电平值
 */
static void I2C_DelayedWriteSDA(const I2C_PortConfig* config, uint8_t bit_value)
{
    I2C_GPIO_WriteSDA_Param(config->gpio_port, config->sda_pin, bit_value);
    if (config->delay_us > 0) {
        Delay_us(config->delay_us);
    }
}

/*============================================================================
 *                          实例管理接口
 *============================================================================*/

uint8_t I2C_CreateInstance(GPIO_TypeDef* gpio_port, uint16_t scl_pin, uint16_t sda_pin, uint8_t delay_us)
{
    if (gpio_port == NULL || scl_pin == 0 || sda_pin == 0) {
        return 0xFF;
    }
    
    if (instance_count >= I2C_MAX_INSTANCES) {
        return 0xFF;
    }

    // === 直接使能 GPIO 时钟，无需检查是否已使能 ===
    if (gpio_port == GPIOA) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);
    } else if (gpio_port == GPIOB) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    } else if (gpio_port == GPIOC) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOC, ENABLE);
    } else if (gpio_port == GPIOD) {
        RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOD, ENABLE);
    } else {
        return 0xFF;
    }

    // 配置 GPIO...
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_OD;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_InitStructure.GPIO_Pin = scl_pin | sda_pin;
    GPIO_Init(gpio_port, &GPIO_InitStructure);

    // 初始化实例...
    i2c_instances[instance_count].gpio_port = gpio_port;
    i2c_instances[instance_count].scl_pin = scl_pin;
    i2c_instances[instance_count].sda_pin = sda_pin;
    i2c_instances[instance_count].delay_us = delay_us;

    I2C_DelayedWriteSCL(&i2c_instances[instance_count], 1);
    I2C_DelayedWriteSDA(&i2c_instances[instance_count], 1);

    return instance_count++;
}

/**
 * @brief 获取指定实例的配置信息
 * 
 * @param instance_id 实例ID（0-3）
 * @return const I2C_PortConfig* 配置结构体指针，NULL表示无效ID
 */
const I2C_PortConfig* I2C_GetInstanceConfig(uint8_t instance_id)
{
    if (instance_id >= instance_count) {
        return NULL;
    }
    return &i2c_instances[instance_id];
}

/*============================================================================
 *                          I2C协议核心时序函数
 *============================================================================*/

/**
 * @brief 为指定实例生成I2C起始信号
 * 
 * I2C通信开始的标志性时序：在SCL高电平时将SDA从高拉低
 * 用于通知总线上所有设备即将开始数据传输
 * 
 * @param instance_id I2C实例ID
 * @note 起始信号后会自动将SCL拉低，为后续数据传输做准备
 * @warning 实例ID必须有效，否则函数直接返回
 */
void I2C_Start_Instance(uint8_t instance_id)
{
    const I2C_PortConfig* config = I2C_GetInstanceConfig(instance_id);
    if (config == NULL) return;
    
    I2C_DelayedWriteSDA(config, 1);       // 释放SDA，确保SDA为高电平
    I2C_DelayedWriteSCL(config, 1);       // 释放SCL，确保SCL为高电平
    I2C_DelayedWriteSDA(config, 0);       // 在SCL高电平期间，拉低SDA，产生起始信号
    I2C_DelayedWriteSCL(config, 0);       // 起始后把SCL也拉低
}

/**
 * @brief 为指定实例生成I2C终止信号
 * 
 * I2C通信结束的标志性时序：在SCL高电平时将SDA从低拉高
 * 用于通知总线上所有设备数据传输已完成
 * 
 * @param instance_id I2C实例ID
 * @note 终止信号后SDA和SCL都会处于高电平状态，总线空闲
 * @warning 实例ID必须有效，否则函数直接返回
 */
void I2C_Stop_Instance(uint8_t instance_id)
{
    const I2C_PortConfig* config = I2C_GetInstanceConfig(instance_id);
    if (config == NULL) return;
    
    I2C_DelayedWriteSDA(config, 0);       // 拉低SDA，确保SDA为低电平
    I2C_DelayedWriteSCL(config, 1);       // 释放SCL，使SCL呈现高电平
    I2C_DelayedWriteSDA(config, 1);       // 在SCL高电平期间，释放SDA，产生终止信号
}

/**
 * @brief 向指定实例发送一个字节数据
 * 
 * 按照I2C协议时序，逐位发送一个字节的数据
 * 从最高位(MSB)开始发送，每个位在SCL高电平期间保持稳定
 * 
 * @param instance_id I2C实例ID
 * @param byte 要发送的字节数据，范围：0x00~0xFF
 * @note 发送完成后会产生一个额外的时钟周期，但不处理应答信号
 * @warning 实例ID必须有效，否则函数直接返回
 */
void I2C_SendByte_Instance(uint8_t instance_id, uint8_t byte)
{
    const I2C_PortConfig* config = I2C_GetInstanceConfig(instance_id);
    if (config == NULL) return;
    
    uint8_t i;
    
    /* 循环8次，主机依次发送数据的每一位 */
    for (i = 0; i < 8; i++) {
        /* 使用掩码的方式取出Byte的指定一位数据并写入到SDA线 */
        I2C_DelayedWriteSDA(config, !!(byte & (0x80 >> i)));
        I2C_DelayedWriteSCL(config, 1);   // 释放SCL，从机在SCL高电平期间读取SDA
        I2C_DelayedWriteSCL(config, 0);   // 拉低SCL，主机开始发送下一位数据
    }
    
    I2C_DelayedWriteSCL(config, 1);       // 额外的一个时钟，不处理应答信号
    I2C_DelayedWriteSCL(config, 0);
}
