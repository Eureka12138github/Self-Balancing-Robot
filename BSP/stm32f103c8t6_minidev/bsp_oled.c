/**
 * @file bsp_oled.c
 * @brief OLED显示驱动硬件层实现文件
 * @author 江协科技
 * @date 2026-02-22
 * 
 * 这个文件是OLED库的硬件层实现，移植时需要根据具体硬件平台修改此文件内容
 */

#include "bsp_oled.h" 
#include "bsp_i2c.h"     // 通用I2C驱动头文件
#include "bsp_config.h"  // 系统配置头文件
#include "my_assert.h"   // 断言机制头文件

/* 通信协议相关定义 ******************************************************/

/** OLED I2C实例ID，用于多实例管理 */
static uint8_t oled_i2c_id = 0;

/**
 * @brief OLED GPIO和I2C接口初始化
 * @return 无
 * @note 初始化OLED使用的GPIO引脚和I2C通信接口
 */
static void OLED_GPIO_Init(void) {
    
    oled_i2c_id = I2C_CreateInstance(OLED_GPIO_PORT, OLED_SCL_PIN, OLED_SDA_PIN, SCL_SDA_DELAY_US);
    OLED_ASSERT(oled_i2c_id != 0xFF);
     
}

/**
 * @brief 向OLED写入命令
 * @param Command 要写入的命令值，范围：0x00~0xFF
 * @return 无
 * @note 通过I2C接口向OLED控制器发送控制命令
 */
void OLED_WriteCommand(uint8_t Command)
{
    I2C_Start_Instance(oled_i2c_id);        // 启动I2C传输
    I2C_SendByte_Instance(oled_i2c_id, 0x78); // 发送OLED设备地址
    I2C_SendByte_Instance(oled_i2c_id, 0x00); // 发送命令控制字节
    I2C_SendByte_Instance(oled_i2c_id, Command); // 发送具体命令
    I2C_Stop_Instance(oled_i2c_id);         // 停止I2C传输
}

/**
 * @brief 向OLED写入显示数据
 * @param Data 要写入数据的起始地址指针
 * @param Count 要写入数据的数量
 * @return 无
 * @note 支持颜色模式切换，根据OLED_ColorMode决定是否取反数据显示
 */
void OLED_WriteData(uint8_t *Data, uint8_t Count)
{
    uint8_t i;
    
    I2C_Start_Instance(oled_i2c_id);        // 启动I2C传输
    I2C_SendByte_Instance(oled_i2c_id, 0x78); // 发送OLED设备地址
    I2C_SendByte_Instance(oled_i2c_id, 0x40); // 发送数据控制字节
    
    /* 循环Count次，进行连续的数据写入 */
    for (i = 0; i < Count; i ++)
    {
        if(OLED_ColorMode){
            I2C_SendByte_Instance(oled_i2c_id, Data[i]);  // 正常模式发送数据
        }else{
            I2C_SendByte_Instance(oled_i2c_id, ~Data[i]); // 反色模式发送数据取反
        }
    }
    I2C_Stop_Instance(oled_i2c_id);         // 停止I2C传输
}

/*********************通信协议结束*****************************************/

/**
 * @brief OLED显示模块初始化
 * @return 无
 * @note 使用OLED显示功能前必须调用此初始化函数
 * @note 包含GPIO初始化、显示参数配置和显存清零操作
 */
void OLED_Init(void)
{
    OLED_GPIO_Init();           // 先调用底层的端口初始化
    
    /* 写入一系列的命令，对OLED进行初始化配置 */
    OLED_WriteCommand(0xAE);    // 设置显示开启/关闭，0xAE关闭，0xAF开启
    
    OLED_WriteCommand(0xD5);    // 设置显示时钟分频比/振荡器频率
    OLED_WriteCommand(0xf0);    // 0x00~0xFF
    
    OLED_WriteCommand(0xA8);    // 设置多路复用率
    OLED_WriteCommand(0x3F);    // 0x0E~0x3F
    
    OLED_WriteCommand(0xD3);    // 设置显示偏移
    OLED_WriteCommand(0x00);    // 0x00~0x7F
    
    OLED_WriteCommand(0x40);    // 设置显示开始行，0x40~0x7F
    
    OLED_WriteCommand(0xA1);    // 设置左右方向，0xA1正常，0xA0左右反置
    
    OLED_WriteCommand(0xC8);    // 设置上下方向，0xC8正常，0xC0上下反置

    OLED_WriteCommand(0xDA);    // 设置COM引脚硬件配置
    OLED_WriteCommand(0x12);
    
    OLED_WriteCommand(0x81);    // 设置对比度
    OLED_WriteCommand(0xDF);    // 0x00~0xFF

    OLED_WriteCommand(0xD9);    // 设置预充电周期
    OLED_WriteCommand(0xF1);

    OLED_WriteCommand(0xDB);    // 设置VCOMH取消选择级别
    OLED_WriteCommand(0x30);

    OLED_WriteCommand(0xA4);    // 设置整个显示打开/关闭

    OLED_WriteCommand(0xA6);    // 设置正常/反色显示，0xA6正常，0xA7反色

    OLED_WriteCommand(0x8D);    // 设置充电泵
    OLED_WriteCommand(0x14);

    OLED_WriteCommand(0xAF);    // 开启显示
    
    // 清空显存数组
    int16_t i, j;
    for (j = 0; j < OLED_HEIGHT/8; j++)                // 遍历8页
    {
        for (i = 0; i < OLED_WIDTH; i++)            // 遍历OLED_WIDTH列
        {
            OLED_DisplayBuf[j][i] = 0x00;    // 将显存数组数据全部清零
        }
    } 
    
    // 更新显示，清屏，防止初始化后未显示内容时花屏
    uint8_t k;
    /* 遍历每一页 */
    for (k = 0; k < 8; k ++)
    {
        /* 设置光标位置为每一页的第一列 */
        OLED_WriteCommand(0xB0 | k);                 // 设置页位置
        OLED_WriteCommand(0x10 | ((0 & 0xF0) >> 4));    // 设置X位置高4位
        OLED_WriteCommand(0x00 | (0 & 0x0F));           // 设置X位置低4位
        /* 连续写入128个数据，将显存数组的数据写入到OLED硬件 */
        OLED_WriteData(OLED_DisplayBuf[k], 128);
    }              
}
