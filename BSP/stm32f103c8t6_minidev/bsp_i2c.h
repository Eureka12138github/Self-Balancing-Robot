/**
 * @file bsp_i2c.h
 * @brief 通用软件模拟I2C总线驱动头文件（支持多实例配置）
 * 
 * 定义了I2C通信所需的基本函数接口和支持多实例的配置结构
 * 通过运行时参数配置可适配不同硬件平台和多个I2C设备
 * 
 * @author Eureka & Lingma
 * @date 2026-02-22
 */

#ifndef BSP_I2C_H
#define BSP_I2C_H

#include "stm32f10x.h"
#include "bsp_config.h"

/*============================================================================
 *                          数据结构定义
 *============================================================================*/

/**
 * @brief I2C端口配置结构体
 * 
 * 用于存储单个I2C实例的硬件配置信息
 */
typedef struct {
    GPIO_TypeDef* gpio_port;    /*!< GPIO端口基地址 */
    uint16_t scl_pin;           /*!< SCL引脚号 */
    uint16_t sda_pin;           /*!< SDA引脚号 */
    uint8_t delay_us;           /*!< 时序延时(微秒)，用于控制通信速度 */
} I2C_PortConfig;

/*============================================================================
 *                          实例管理接口
 *============================================================================*/
uint8_t I2C_CreateInstance(GPIO_TypeDef* gpio_port, uint16_t scl_pin, uint16_t sda_pin, uint8_t delay_us);
const I2C_PortConfig* I2C_GetInstanceConfig(uint8_t instance_id);

/*============================================================================
 *                          I2C协议核心时序函数（多实例版本）
 *============================================================================*/

void I2C_Start_Instance(uint8_t instance_id);
void I2C_Stop_Instance(uint8_t instance_id);
void I2C_SendByte_Instance(uint8_t instance_id, uint8_t byte);


#endif
