/**
 * @file dht11.c
 * @brief DHT11 温湿度传感器驱动（基于 STM32F10x 标准外设库）
 * @note 采用单总线协议，需动态切换 GPIO 模式（输出/浮空输入）
 *       严格遵循 DHT11 官方通信时序（主机复位 → 从机响应 → 40bit 数据）
 */

#include "dht11.h"
#include "delay.h"
#include "bsp_config.h"
/**
 * @brief 将 DHT11 数据引脚配置为推挽输出模式
 * @note 用于主机发送复位信号和释放总线
 */
void DHT11_GPIO_Init_OUT(void)
{
    RCC_APB2PeriphClockCmd(DHT11_GPIO_CLK, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_Out_PP;   // 推挽输出
    GPIO_InitStructure.GPIO_Pin   = DHT11_PIN;         // 用户定义的引脚
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;   // 最高速度以满足时序
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

/**
 * @brief 将 DHT11 数据引脚配置为浮空输入模式
 * @note 用于接收 DHT11 返回的响应信号和数据位
 *       浮空输入可避免内部上拉干扰单总线电平
 */
void DHT11_GPIO_Init_IN(void)
{
    RCC_APB2PeriphClockCmd(DHT11_GPIO_CLK, ENABLE);
    
    GPIO_InitTypeDef GPIO_InitStructure;
    GPIO_InitStructure.GPIO_Mode  = GPIO_Mode_IN_FLOATING; // 浮空输入
    GPIO_InitStructure.GPIO_Pin   = DHT11_PIN;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
    GPIO_Init(DHT11_GPIO_PORT, &GPIO_InitStructure);
}

/**
 * @brief DHT11 初始化：发送复位信号并检测设备是否存在
 * @return true  表示 DHT11 响应正常，初始化成功
 * @return false 表示未检测到 DHT11 或通信失败
 * @note 必须在调用前确保 delay 函数已正确初始化（SysTick 或 TIM）
 */
bool DHT11_Init(void)
{
    DHT11_GPIO_Init_OUT();
    DHT11_HIGH();      // 确保初始为高电平
    DHT11_Rst();       // 发送复位脉冲
    return DHT11_Check(); // 检测从机响应
}

/**
 * @brief 向 DHT11 发送复位信号（主机启动信号）
 * @note 时序要求：
 *       - 拉低 ≥18ms（此处使用 25ms 保证裕量）
 *       - 拉高 20~40μs（此处使用 30μs）
 */
void DHT11_Rst(void)
{
    DHT11_GPIO_Init_OUT();
    DHT11_LOW();        // 主机拉低总线
    Delay_ms(25);       // 保持低电平至少 18ms
    DHT11_HIGH();       // 释放总线（上拉电阻拉高）
    Delay_us(30);       // 延迟 20~40μs，等待 DHT11 响应
}

/**
 * @brief 检测 DHT11 是否存在并完成响应握手
 * @return true  成功接收到 80us 低 + 80us 高 的响应信号
 * @return false 超时未收到响应，可能设备未连接或损坏
 * @note 响应时序：
 *       - DHT11 拉低总线 80μs（表示开始响应）
 *       - 再拉高 80μs（表示准备发送数据）
 */
bool DHT11_Check(void)
{
    uint8_t retry = 0;

    // 等待 DHT11 拉低总线（80us 低电平）
    DHT11_GPIO_Init_IN();
    while (!DHT11_READ() && retry < 100)
    {
        retry++;
        Delay_us(1);
    }
    if (retry >= 100) return false; // 超时：未检测到低电平

    retry = 0;
    // 等待 DHT11 拉高总线（80us 高电平）
    while (DHT11_READ() && retry < 100)
    {
        retry++;
        Delay_us(1);
    }
    if (retry >= 100) return false; // 超时：未检测到高电平

    return true; // 响应完整，设备存在
}

/**
 * @brief 从 DHT11 读取一个数据位（bit）
 * @return 1 或 0，表示当前位的值
 * @note 数据位时序：
 *       - 50us 低电平起始
 *       - 高电平持续时间决定 bit 值：
 *           - 26~28us → 0
 *           - 70us     → 1
 *       - 此处延迟 40us 后采样，可可靠区分两种电平
 */
uint8_t DHT11_Read_Bit(void)
{
    uint8_t retry = 0;

    // 等待 50us 低电平结束
    while (DHT11_READ() && retry < 100)
    {
        retry++;
        Delay_us(1);
    }

    retry = 0;
    // 等待高电平开始
    while (!DHT11_READ() && retry < 100)
    {
        retry++;
        Delay_us(1);
    }

    Delay_us(40); // 在 26~70us 区间中间采样（40us 安全点）

    return DHT11_READ() ? 1 : 0;
}

/**
 * @brief 从 DHT11 读取一个字节（8 位）
 * @return 读取到的 8 位数据（MSB 先传）
 */
uint8_t DHT11_Read_Byte(void)
{
    uint8_t i, dat = 0;
    for (i = 0; i < 8; i++)
    {
        dat <<= 1;
        dat |= DHT11_Read_Bit();
    }
    return dat;
}

/**
 * @brief 从 DHT11 读取完整温湿度数据（40 位）
 * @param[out] temp 指向温度值的指针（单位：℃，整数部分）
 * @param[out] humi 指向湿度值的指针（单位：%RH，整数部分）
 * @return true  数据校验通过，读取成功
 * @return false 通信失败或校验错误
 * @note 数据格式（5 字节）：
 *       - Byte0: 湿度整数
 *       - Byte1: 湿度小数（DHT11 固为 0）
 *       - Byte2: 温度整数
 *       - Byte3: 温度小数（DHT11 固为 0）
 *       - Byte4: 校验和 = Byte0+Byte1+Byte2+Byte3
 *       - 本驱动仅返回整数部分（符合 DHT11 实际精度）
 */
bool DHT11_Read_Data(uint16_t *temp, uint16_t *humi)
{
    uint8_t buf[5];
    uint8_t i;

    DHT11_Rst();
    if (!DHT11_Check())
    {
        return false; // 无响应
    }

    // 读取 5 字节（40 位）数据
    for (i = 0; i < 5; i++)
    {
        buf[i] = DHT11_Read_Byte();
    }

    // 校验和验证
    if ((buf[0] + buf[1] + buf[2] + buf[3]) == buf[4])
    {
        *humi = buf[0]; // 湿度整数部分
        *temp = buf[2]; // 温度整数部分
        return true;
    }

    return false; // 校验失败
}
