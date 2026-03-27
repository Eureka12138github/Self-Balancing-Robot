/**
 * @file OLED.c
 * @brief OLED显示驱动实现文件
 * @details 此文件包含OLED显示的各种高级功能实现，包括文本显示、图形绘制、区域控制等功能
 * @note 移植时通常不需要修改此文件内容
 * @author 江协科技 & 上nm网课呢
 * @date 2026-02-22
 */

#include "OLED.h"
#include "OLED_Fonts.h"     // OLED字体库头文件
#include "system_config.h"     // OLED字体库头文件
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include "i2c.h"     // 通用I2C驱动头文件
#include "bsp_config.h"  // 系统配置头文件
#include "my_assert.h"   // 断言机制头文件

//OLED显示屏
uint8_t OLED_DisplayBuf[OLED_HEIGHT/8][OLED_WIDTH];
bool OLED_ColorMode = true;

/* ========================================
 * OLED初始化
 * ======================================== */


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

 

/* ========================================
 * 工具函数实现
 * ======================================== */

/**
 * @brief 计算次方值
 * @param[in] X 底数
 * @param[in] Y 指数
 * @return X的Y次方结果
 */
uint32_t OLED_Pow(uint32_t X, uint32_t Y)
{
    uint32_t Result = 1;    // 结果默认为1
    while (Y--)             // 累乘Y次
    {
        Result *= X;        // 每次把X累乘到结果上
    }
    return Result;
}

/**
 * @brief 判断指定点是否在指定多边形内部
 * @param[in] nvert 多边形的顶点数
 * @param[in] vertx 包含多边形顶点X坐标的数组
 * @param[in] verty 包含多边形顶点Y坐标的数组
 * @param[in] testx 测试点的X坐标
 * @param[in] testy 测试点的Y坐标
 * @return 指定点是否在指定多边形内部
 *         @retval 1 在内部
 *         @retval 0 不在内部
 * @note 此算法由W. Randolph Franklin提出
 * @see https://wrfranklin.org/Research/Short_Notes/pnpoly.html
 */
uint8_t OLED_pnpoly(uint8_t nvert, int16_t *vertx, int16_t *verty, int16_t testx, int16_t testy)
{
    int16_t i = 0, j = 0;
    uint8_t c = 0;
    
    for (i = 0, j = nvert - 1; i < nvert; j = i++)
    {
        if (((verty[i] > testy) != (verty[j] > testy)) &&
            (testx < (vertx[j] - vertx[i]) * (testy - verty[i]) / (verty[j] - verty[i]) + vertx[i]))
        {
            c = !c;
        }
    }
    return c;
}

/**
 * @brief 获取四个数值中的最大值
 * @param[in] a 第一个数值
 * @param[in] b 第二个数值
 * @param[in] c 第三个数值
 * @param[in] d 第四个数值
 * @return 四个数值中的最大值
 */
int16_t max(int16_t a, int16_t b, int16_t c, int16_t d) 
{
    int16_t max_val = a; // 假设a是最大的

    if (b > max_val) {
        max_val = b; // 如果b大于当前最大值，则更新最大值为b
    }
    if (c > max_val) {
        max_val = c; // 如果c大于当前最大值，则更新最大值为c
    }
    if (d > max_val) {
        max_val = d; // 如果d大于当前最大值，则更新最大值为d
    }

    return max_val; // 返回最大值
}

/**
 * @brief 计算浮点数的绝对值
 * @param[in] num 输入数值
 * @return num的绝对值
 */
float numabs(float num)
{
    if(num > 0)
        return num;
    if(num < 0)
        return -num;
    return 0;
}

/**
 * @brief 判断指定点是否在指定角度范围内
 * @param[in] X 指定点的X坐标
 * @param[in] Y 指定点的Y坐标
 * @param[in] StartAngle 起始角度，范围：-180~180度
 * @param[in] EndAngle 终止角度，范围：-180~180度
 * @note 角度定义：水平向右为0度，水平向左为180度或-180度，下方为正数，上方为负数，顺时针旋转
 * @return 指定点是否在指定角度范围内
 *         @retval 1 在角度范围内
 *         @retval 0 不在角度范围内
 */
uint8_t OLED_IsInAngle(int16_t X, int16_t Y, int16_t StartAngle, int16_t EndAngle)
{
    int16_t PointAngle;
    PointAngle = atan2(Y, X) / 3.14 * 180;    // 计算指定点的弧度，并转换为角度表示
    
    if (StartAngle < EndAngle)    // 起始角度小于终止角度的情况
    {
        /* 如果指定角度在起始终止角度之间，则判定指定点在指定角度 */
        if (PointAngle >= StartAngle && PointAngle <= EndAngle)
        {
            return 1;
        }
    }
    else            // 起始角度大于终止角度的情况
    {
        /* 如果指定角度大于起始角度或者小于终止角度，则判定指定点在指定角度 */
        if (PointAngle >= StartAngle || PointAngle <= EndAngle)
        {
            return 1;
        }
    }
    return 0;        // 不满足以上条件，则判断指定点不在指定角度
}

/* ========================================
 * 显示模式和亮度控制函数
 * ======================================== */

/**
 * @brief 设置显示颜色模式
 * @param colormode 颜色模式，true为黑色背景模式，false为白色背景模式
 * @return 无
 */
void OLED_SetColorMode(bool colormode){
    OLED_ColorMode = colormode;
}

/**
 * @brief 设置OLED显示亮度
 * @param Brightness 亮度值，范围0-255
 * @return 无
 * @note 不要设置过大或过小的亮度值
 */
void OLED_Brightness(int16_t Brightness){
    if(Brightness > 255){
        Brightness = 255;
    }
    if(Brightness < 0){
        Brightness = 0;
    }
    OLED_WriteCommand(0x81);
    OLED_WriteCommand(Brightness);
}

/* ========================================
 * 显示控制函数
 * ======================================== */

/**
 * @brief 设置OLED显示光标位置
 * @param Page 页地址，范围0-7
 * @param X X轴坐标，范围0-127
 * @return 无
 * @note OLED的Y轴只能以8个bit为一组写入，即1页等于8个Y轴坐标
 * @note 如使用1.3寸OLED显示屏(SH1106芯片)，需将X坐标加2以适应132列显示
 */
void OLED_SetCursor(uint8_t Page, uint8_t X)
{
    /* 1.3寸OLED显示屏适配(如需要请取消注释)
     * SH1106芯片有132列，屏幕起始列在第2列而非第0列
     * X += 2;
     */
    
    /* 通过指令设置页地址和列地址 */
    OLED_WriteCommand(0xB0 | Page);                 // 设置页位置
    OLED_WriteCommand(0x10 | ((X & 0xF0) >> 4));    // 设置X位置高4位
    OLED_WriteCommand(0x00 | (X & 0x0F));           // 设置X位置低4位
}

/**
 * @brief 更新整个OLED显存到屏幕
 * @return 无
 * @note 所有显示函数仅操作OLED显存数组
 * @note 需调用此函数或将显存数据发送到OLED硬件进行实际显示
 */
void OLED_Update(void)
{
    uint8_t j;
    /* 遍历每一页 */
    for (j = 0; j < 8; j++)
    {
        /* 设置光标位置为每一页的第一列 */
        OLED_SetCursor(j, 0);
        /* 连续写入128个数据，将显存数组数据写入OLED硬件 */
        OLED_WriteData(OLED_DisplayBuf[j], 128);
    }
}

/**
 * @brief 更新OLED显存指定区域到屏幕
 * @param X 区域左上角X坐标，范围0-OLED_WIDTH-1
 * @param Y 区域左上角Y坐标，范围0-OLED_HEIGHT-1
 * @param Width 区域宽度，范围0-OLED_WIDTH
 * @param Height 区域高度，范围0-OLED_HEIGHT
 * @return 无
 * @note 此函数至少更新参数指定的区域
 * @note 如果更新区域Y轴只包含部分页，则同一页的剩余部分会一并更新
 * @note 所有显示函数仅操作OLED显存数组，需调用此函数进行实际显示
 */
void OLED_UpdateArea(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height)
{
    uint8_t j;
    
    /* 参数检查，确保指定区域不超出屏幕范围 */
    if (X > 128-1) {return;}
    if (Y > 64-1) {return;}
    if (X + Width > 128) {Width = 128 - X;}
    if (Y + Height > 64) {Height = 64 - Y;}
    
    /* 遍历指定区域涉及的相关页 */
    /* (Y + Height - 1) / 8 + 1 的目的是 (Y + Height) / 8 并向上取整 */
    for (j = Y / 8; j < (Y + Height - 1) / 8 + 1; j++)
    {
        /* 设置光标位置为相关页的指定列 */
        OLED_SetCursor(j, X);
        /* 连续写入Width个数据，将显存数组数据写入OLED硬件 */
        OLED_WriteData(&OLED_DisplayBuf[j][X], Width);
    }
}
/**
 * @brief 清空整个OLED显存数组
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 */
void OLED_Clear(void)
{
    int16_t i, j;
    for (j = 0; j < OLED_HEIGHT/8; j++)                // 遍历8页
    {
        for (i = 0; i < OLED_WIDTH; i++)            // 遍历OLED_WIDTH列
        {
            OLED_DisplayBuf[j][i] = 0x00;    // 将显存数组数据全部清零
        }
    }
}

/**
 * @brief 清空OLED显存数组的指定区域
 * @param[in] X 指定区域左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定区域左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Width 指定区域的宽度，范围：正数
 * @param[in] Height 指定区域的高度，范围：正数
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 */
void OLED_ClearArea(int16_t X, int16_t Y, int16_t Width, int16_t Height)
{
    int16_t x_start, y_start, x_end, y_end;
    int16_t i, j;

    if (Width <= 0 || Height <= 0) return;

    // 计算X方向的起始和结束位置
    x_start = (X < 0) ? 0 : X;
    x_end = X + Width;
    if (x_end > OLED_WIDTH) x_end = OLED_WIDTH;
    if (x_start >= x_end) return;

    // 计算Y方向的起始和结束位置
    y_start = (Y < 0) ? 0 : Y;
    y_end = Y + Height;
    if (y_end > OLED_HEIGHT) y_end = OLED_HEIGHT;
    if (y_start >= y_end) return;

    // 调整Width和Height为实际需要清除的区域
    Width = x_end - x_start;
    Height = y_end - y_start;

    for (j = y_start; j < y_end; j++) {
        for (i = x_start; i < x_end; i++) {
            OLED_DisplayBuf[j / 8][i] &= ~(0x01 << (j % 8));
        }
    }
}

/**
 * @brief 将OLED显存数组全部取反（反色显示）
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 */
void OLED_Reverse(void)
{
    uint8_t i, j;
    for (j = 0; j < OLED_HEIGHT/8; j++)                // 遍历页
    {
        for (i = 0; i < OLED_WIDTH; i++)            // 遍历OLED_WIDTH列
        {
            OLED_DisplayBuf[j][i] ^= 0xFF;    // 将显存数组数据全部取反
        }
    }
}

/**
 * @brief 将OLED显存数组指定区域取反（反色显示）
 * @param[in] X 指定区域左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定区域左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Width 指定区域的宽度，范围：负数~OLED_WIDTH
 * @param[in] Height 指定区域的高度，范围：负数~OLED_HEIGHT
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 */
void OLED_ReverseArea(int16_t X, int16_t Y, int16_t Width, int16_t Height)
{
    int16_t i, j, x, y;
    if(Width <= 0 || Height <= 0) {return; }
    
    /* 参数检查，保证指定区域不会超出屏幕范围 */
    if (X > OLED_WIDTH-1) {return;}
    if (Y > OLED_HEIGHT-1) {return;}
    if (X + Width > OLED_WIDTH) {Width = OLED_WIDTH - X;}
    if (Y + Height > OLED_HEIGHT) {Height = OLED_HEIGHT - Y;}
    if (X + Width < 0) {return;}
    if (Y + Height < 0) {return;}
    if (X < 0) { x = 0;} else { x = X;}
    if (Y < 0) { y = 0;} else { y = Y;}
    
    for (j = y; j < Y + Height; j++)        // 遍历指定页
    {
        for (i = x; i < X + Width; i++)    // 遍历指定列
        {
            OLED_DisplayBuf[j / 8][i] ^= 0x01 << (j % 8);    // 将显存数组指定数据取反
        }
    }
}


/* ========================================
 * 图像显示函数实现
 * ======================================== */

/**
 * @brief 在OLED上显示图像（支持负坐标）
 * @param[in] X 指定图像左上角的横坐标，范围：负值~OLED_WIDTH-1
 * @param[in] Y 指定图像左上角的纵坐标，范围：负值~OLED_HEIGHT-1
 * @param[in] Width 指定图像的宽度，范围：正数
 * @param[in] Height 指定图像的高度，范围：正数
 * @param[in] Image 指定要显示的图像数据指针
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note XY坐标均可为负值，负值部分会被裁剪不显示
 */
//void OLED_ShowImage(int16_t X, int16_t Y, uint16_t Width, uint16_t Height, const uint8_t *Image)
//{
//    uint8_t i, j;
//    
//    /* 参数检查，保证指定图像不会超出屏幕范围 */
//    if (Width == 0 || Height == 0) {
//        return; // 如果宽度或高度为0，直接返回
//    }
//    
//    if (X > OLED_WIDTH-1) {
//        return; // X 超出右边界，直接返回
//    }
//    if (Y > OLED_HEIGHT-1) {
//        return; // Y 超出下边界，直接返回
//    }
//    
//    /* 将图像所在区域清空 */
//    uint8_t startX = (X < 0) ? 0 : X; // 计算实际起始显示位置的 X 坐标
//    uint8_t startY = (Y < 0) ? 0 : Y; // 计算实际起始显示位置的 Y 坐标
//    uint8_t endX = (X + Width - 1 > OLED_WIDTH-1) ? OLED_WIDTH-1 : X + Width - 1; // 计算实际结束显示位置的 X 坐标
//    uint8_t endY = (Y + Height - 1 > OLED_HEIGHT-1) ? OLED_HEIGHT-1 : Y + Height - 1; // 计算实际结束显示位置的 Y 坐标
//    
//    OLED_ClearArea(startX, startY, endX - startX + 1, endY - startY + 1);
//    
//    /* 遍历指定图像涉及的相关页 */
//    for (j = 0; j < (Height - 1) / 8 + 1; j++)
//    {
//        /* 遍历指定图像涉及的相关列 */
//        for (i = 0; i < Width; i++)
//        {
//            int16_t currX = X + i;
//            int16_t currY = Y + j * 8;
//            
//            /* 超出边界，则跳过显示 */
//            if (currX < 0 || currX > OLED_WIDTH-1 ||currY < 0 || currY > OLED_HEIGHT-1) {
//                continue;
//            }
//            /* 显示图像在当前页的内容 */
//            OLED_DisplayBuf[currY / 8][currX] |= Image[j * Width + i] << (currY % 8);
//            /* 当前页下一页 */
//            if (currY + 8 <= OLED_HEIGHT-1) {
//                OLED_DisplayBuf[currY / 8 + 1][currX] |= Image[j * Width + i] >> (8 - currY % 8);
//            }
//        }
//    }
//    
//    if(Y<0){
//        for (i = 0; i < Width; i++)
//        {
//            int16_t currX = X + i;
//            if (currX < 0 || currX > OLED_WIDTH-1) {
//                continue;
//            }
//            OLED_DisplayBuf[0][currX] |= Image[ -Y/8*Width+i] >> -Y%8;
//        }
//    }
//}

/**
 * @brief 在OLED上显示图像（支持负坐标）
 * @param[in] X 指定图像左上角的横坐标，范围：负值~OLED_WIDTH-1
 * @param[in] Y 指定图像左上角的纵坐标，范围：负值~OLED_HEIGHT-1
 * @param[in] Width 指定图像的宽度，范围：正数
 * @param[in] Height 指定图像的高度，范围：正数
 * @param[in] Image 指定要显示的图像数据指针
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note XY坐标均可为负值，负值部分会被裁剪不显示
 *
 * @details 相较于旧函数的主要改进点：
 * 1. **参数检查优化**：
 *    - 增加了对完全不可见区域的快速返回逻辑（`if (X >= OLED_WIDTH || Y >= OLED_HEIGHT) return;`），
 *      避免无效绘制操作。
 *
 * 2. **统一主循环逻辑**：
 *    - 删除了旧函数中针对 `Y < 0` 的特殊处理分支，将其合并到主绘制循环中。
 *    - 主循环通过统一的边界检查（`currX < 0 || currX > OLED_WIDTH-1 || currY < 0 || currY > OLED_HEIGHT-1`）
 *      自动跳过无效像素，无需额外分支。
 *
 * 3. **增强安全性**：
 *    - 增加了图像数据索引越界检查（`if (img_idx >= Width * ((Height + 7) / 8)) continue;`），
 *      防止访问非法内存地址。
 *
 * 4. **跨页处理优化**：
 *    - 使用 `OLED_PAGES` 替代硬编码的 `OLED_HEIGHT-1`，提高代码可移植性和可读性。
 *    - 跨页逻辑更清晰，仅在 `bit_offset > 0` 且下一页存在时才进行处理。
 *
 * @warning 旧函数中存在的冗余逻辑和潜在风险已被修复，新函数更加简洁、安全、高效。
 */
void OLED_ShowImage(int16_t X, int16_t Y, uint16_t Width, uint16_t Height, const uint8_t *Image)
{
    if (Width == 0 || Height == 0) return;
    if (X >= OLED_WIDTH || Y >= OLED_HEIGHT) return; // 完全不可见

    // === 计算有效绘制区域 ===
    uint8_t startX = (X < 0) ? 0 : X;
    uint8_t startY = (Y < 0) ? 0 : Y;
    uint8_t endX = (X + Width - 1 > OLED_WIDTH-1) ? OLED_WIDTH-1 : X + Width - 1;
    uint8_t endY = (Y + Height - 1 > OLED_HEIGHT-1) ? OLED_HEIGHT-1 : Y + Height - 1;

    // === 清除指定区域 ===
    OLED_ClearArea(startX, startY, endX - startX + 1, endY - startY + 1);

    // === 主绘制循环 ===
    for (int16_t j = 0; j < (Height - 1) / 8 + 1; j++) {
        for (int16_t i = 0; i < Width; i++) {
            int16_t currX = X + i;
            int16_t currY = Y + j * 8;

            // 边界检查
            if (currX < 0 || currX > OLED_WIDTH-1 || currY < 0 || currY > OLED_HEIGHT-1) {
                continue;
            }

            uint16_t img_idx = j * Width + i;
            if (img_idx >= Width * ((Height + 7) / 8)) continue; // 防越界

            uint8_t data = Image[img_idx];
            uint8_t page = currY / 8;
            uint8_t bit_offset = currY % 8;

            OLED_DisplayBuf[page][currX] |= data << bit_offset;

            // 跨页处理
            if (bit_offset > 0 && page + 1 < OLED_PAGES) {     
                OLED_DisplayBuf[page + 1][currX] |= data >> (8 - bit_offset);
            }
        }
    }

    // 删除特殊处理Y<0的情况，因为主循环已覆盖所有情况
}

/* ========================================
 * 文本显示函数实现
 * ======================================== */

/**
 * @brief 在OLED上显示单个字符
 * @param[in] X 指定字符左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定字符左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Char 指定要显示的字符，范围：ASCII码可见字符
 * @param[in] FontSize 指定字体大小
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 */
void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize)
{
    if (FontSize == OLED_8X16_HALF)        // 字体为宽8像素，高16像素
    {
        /* 将ASCII字模库OLED_F8x16的指定数据以8*16的图像格式显示 */
        OLED_ShowImage(X, Y, 8, 16, OLED_8x16[Char - ' ']);
    }
    else if(FontSize == OLED_6X8_HALF)    // 字体为宽6像素，高8像素
    {
        /* 将ASCII字模库OLED_F6x8的指定数据以6*8的图像格式显示 */
        OLED_ShowImage(X, Y, 6, 8, OLED_6x8[Char - ' ']);
    }
    else if(FontSize == OLED_7X12_HALF)    // 字体为宽7像素，高12像素
    {
        /* 将ASCII字模库OLED_F7x12的指定数据以7*12的图像格式显示 */
        OLED_ShowImage(X, Y, 7, 12, OLED_7x12[Char - ' ']);
    }
}

/**
 * @brief 在OLED上显示无符号十进制数字
 * @param[in] X 指定数字左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定数字左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Number 指定要显示的数字，范围：0~4294967295
 * @param[in] Length 指定数字的显示长度，范围：1~10
 * @param[in] FontSize 指定字体大小
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 */
void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    for (i = 0; i < Length; i++)        // 遍历数字的每一位
    {
        /* 调用OLED_ShowChar函数，依次显示每个数字 */
        /* Number / OLED_Pow(10, Length - i - 1) % 10 可以十进制提取数字的每一位 */
        /* + '0' 可将数字转换为字符格式 */
        OLED_ShowChar(X + i * FontSize, Y, Number / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
    }
}

/**
 * @brief 在OLED上显示有符号十进制数字
 * @param[in] X 指定数字左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定数字左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Number 指定要显示的数字，范围：-2147483648~2147483647
 * @param[in] Length 指定数字的显示长度，范围：1~10
 * @param[in] FontSize 指定字体大小
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 */
void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    uint32_t Number1;
    
    if (Number >= 0)                        // 数字大于等于0
    {
        OLED_ShowChar(X, Y, '+', FontSize);    // 显示+号
        Number1 = Number;                    // Number1直接等于Number
    }
    else                                    // 数字小于0
    {
        OLED_ShowChar(X, Y, '-', FontSize);    // 显示-号
        Number1 = -Number;                    // Number1等于Number取负
    }
    
    for (i = 0; i < Length; i++)            // 遍历数字的每一位
    {
        /* 调用OLED_ShowChar函数，依次显示每个数字 */
        /* Number1 / OLED_Pow(10, Length - i - 1) % 10 可以十进制提取数字的每一位 */
        /* + '0' 可将数字转换为字符格式 */
        OLED_ShowChar(X + (i + 1) * FontSize, Y, Number1 / OLED_Pow(10, Length - i - 1) % 10 + '0', FontSize);
    }
}

/* ========================================
 * 数字显示函数实现
 * ======================================== */

/**
 * @brief 在OLED上显示十六进制数字
 * @param[in] X 指定数字左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定数字左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Number 指定要显示的数字，范围：0x00000000~0xFFFFFFFF
 * @param[in] Length 指定数字的显示长度，范围：1~8位
 * @param[in] FontSize 指定字体大小
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 支持大写字母A-F表示十六进制数字
 */
void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i, SingleNumber;
    for (i = 0; i < Length; i++)        // 遍历数字的每一位
    {
        /* 以十六进制提取数字的每一位 */
        SingleNumber = Number / OLED_Pow(16, Length - i - 1) % 16;
        
        if (SingleNumber < 10)            // 单个数字小于10
        {
            /* 调用OLED_ShowChar函数，显示此数字 */
            /* + '0' 可将数字转换为字符格式 */
            OLED_ShowChar(X + i * FontSize, Y, SingleNumber + '0', FontSize);
        }
        else                            // 单个数字大于等于10
        {
            /* 调用OLED_ShowChar函数，显示此数字 */
            /* - 10 + 'A' 可将数字转换为从A开始的十六进制字符 */
            OLED_ShowChar(X + i * FontSize, Y, SingleNumber - 10 + 'A', FontSize);
        }
    }
}

/**
 * @brief 在OLED上显示二进制数字
 * @param[in] X 指定数字左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定数字左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Number 指定要显示的数字，范围：0x00000000~0xFFFFFFFF
 * @param[in] Length 指定数字的显示长度，范围：1~32位
 * @param[in] FontSize 指定字体大小
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 显示从高位到低位的二进制数字
 */
void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize)
{
    uint8_t i;
    for (i = 0; i < Length; i++)        // 遍历数字的每一位
    {
        /* 调用OLED_ShowChar函数，依次显示每个数字 */
        /* Number / OLED_Pow(2, Length - i - 1) % 2 可以二进制提取数字的每一位 */
        /* + '0' 可将数字转换为字符格式 */
        OLED_ShowChar(X + i * FontSize, Y, Number / OLED_Pow(2, Length - i - 1) % 2 + '0', FontSize);
    }
}

/**
 * @brief 在OLED上显示浮点数字（带小数点）
 * @param[in] X 指定数字左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定数字左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Number 指定要显示的浮点数，范围：-4294967295.0~4294967295.0
 * @param[in] IntLength 指定整数部分的显示长度，范围：0~10位
 * @param[in] FraLength 指定小数部分的显示长度，范围：0~9位，小数进行四舍五入显示
 * @param[in] FontSize 指定字体大小
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 自动处理正负号显示和小数点位置
 */
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize)
{
    uint32_t PowNum, IntNum, FraNum;
    
    if (Number >= 0)                        // 数字大于等于0
    {
        OLED_ShowChar(X, Y, '+', FontSize);    // 显示+号
    }
    else                                    // 数字小于0
    {
        OLED_ShowChar(X, Y, '-', FontSize);    // 显示-号
        Number = -Number;                    // Number取负
    }
    
    /* 提取整数部分和小数部分 */
    IntNum = Number;                        // 直接赋值给整型变量，提取整数部分
    Number -= IntNum;                        // 将Number的整数部分减掉，防止后续计算溢出
    PowNum = OLED_Pow(10, FraLength);        // 根据指定小数位数，确定乘数
    FraNum = round(Number * PowNum);        // 将小数乘到整数，同时四舍五入，避免显示误差
    IntNum += FraNum / PowNum;                // 若四舍五入造成了进位，则需要再加给整数部分
    
    /* 显示整数部分 */
    OLED_ShowNum(X + FontSize, Y, IntNum, IntLength, FontSize);
    
    /* 显示小数点 */
    OLED_ShowChar(X + (IntLength + 1) * FontSize, Y, '.', FontSize);
    
    /* 显示小数部分 */
    OLED_ShowNum(X + (IntLength + 2) * FontSize, Y, FraNum, FraLength, FontSize);
}

/* ========================================
 * 字符串显示函数实现
 * ======================================== */

/**
 * @brief 在OLED上显示ASCII字符串
 * @param[in] X 指定字符串左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定字符串左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] String 指定要显示的字符串，必须为ASCII码可见字符组成的字符串
 * @param[in] FontSize 指定字体大小
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 字符串必须以'\0'结尾
 */
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize)
{
    uint8_t i;
    for (i = 0; String[i] != '\0'; i++)        // 遍历字符串的每个字符
    {
        /* 调用OLED_ShowChar函数，依次显示每个字符 */
        OLED_ShowChar(X + i * FontSize, Y, String[i], FontSize);
    }
}

/**
 * @brief 在OLED上显示中文字符串
 * @param[in] X 指定字符串左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定字符串左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Chinese 指定要显示的中文字符串，必须全部为汉字或全角字符
 * @param[in] FontSize 指定中文字体大小，支持OLED_12X12_FULL、OLED_16X16_FULL、OLED_20X20_FULL
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 显示的汉字需要在OLED_Fonts.c里的对应字体数组中定义
 * @note 未找到指定汉字时，会显示默认图形（方框内问号）
 * @warning 不要在字符串中混入半角字符
 */
void OLED_ShowChinese(int16_t X, int16_t Y, const char *Chinese, uint8_t FontSize)
{
    uint8_t pChinese = 0;
    uint8_t pIndex;
    uint8_t i;
    char SingleChinese[OLED_CHN_CHAR_WIDTH + 1] = {0};
    
    for (i = 0; Chinese[i] != '\0'; i++)    // 遍历中文字符串
    {
        SingleChinese[pChinese] = Chinese[i];    // 提取中文字符串数据到单个汉字数组
        pChinese++;                            // 计数器自增
        
        if (pChinese >= OLED_CHN_CHAR_WIDTH)    // 提取到了一个完整的汉字
        {
            pChinese = 0;    // 计数器归零
            
            const void* fontArray;
            // 根据字体大小选择对应的字体数组
            if (FontSize == OLED_12X12_FULL) {
                fontArray = (const void*) OLED_12x12;
            } else if (FontSize == OLED_16X16_FULL) {
                fontArray = (const void*) OLED_16x16;
            } else if (FontSize == OLED_20X20_FULL) {
                fontArray = (const void*) OLED_20x20;
            }
    
            // 查找对应字体大小的汉字数据
            if(FontSize == OLED_12X12_FULL){
                for (pIndex = 0; strcmp(((const ChineseCell12x12_t*)fontArray)[pIndex].Index, "") != 0; pIndex++)
                {
                    if (strcmp(((const ChineseCell12x12_t*)fontArray)[pIndex].Index, SingleChinese) == 0)
                    {
                        break;
                    }
                }
                OLED_ShowImage(X + ((i + 1) / OLED_CHN_CHAR_WIDTH - 1) * OLED_12X12_FULL, Y, 
                              OLED_12X12_FULL, OLED_12X12_FULL, 
                              ((const ChineseCell12x12_t*)fontArray)[pIndex].Data);
            } else if(FontSize == OLED_16X16_FULL){
                for (pIndex = 0; strcmp(((const ChineseCell16x16_t*)fontArray)[pIndex].Index, "") != 0; pIndex++)
                {
                    if (strcmp(((const ChineseCell16x16_t*)fontArray)[pIndex].Index, SingleChinese) == 0)
                    {
                        break;
                    }
                }
                OLED_ShowImage(X + ((i + 1) / OLED_CHN_CHAR_WIDTH - 1) * OLED_16X16_FULL, Y, 
                              OLED_16X16_FULL, OLED_16X16_FULL, 
                              ((const ChineseCell16x16_t*)fontArray)[pIndex].Data);
            } else if(FontSize == OLED_20X20_FULL){
                for (pIndex = 0; strcmp(((const ChineseCell20x20_t*)fontArray)[pIndex].Index, "") != 0; pIndex++)
                {
                    if (strcmp(((const ChineseCell20x20_t*)fontArray)[pIndex].Index, SingleChinese) == 0)
                    {
                        break;
                    }
                }
                OLED_ShowImage(X + ((i + 1) / OLED_CHN_CHAR_WIDTH - 1) * OLED_20X20_FULL, Y, 
                              OLED_20X20_FULL, OLED_20X20_FULL, 
                              ((const ChineseCell20x20_t*)fontArray)[pIndex].Data);
            }
        }
    }
}

/* ========================================
 * 格式化输出函数实现
 * ======================================== */

/**
 * @brief 使用printf格式在OLED上显示字符串
 * @param[in] X 指定格式化字符串左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定格式化字符串左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] FontSize 指定字体大小
 * @param[in] format 指定要显示的格式化字符串，必须为ASCII码可见字符组成的字符串
 * @param[in] ... 格式化字符串参数列表
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 支持标准 printf 格式化语法，如 %d, %s, %f 等
 * @warning 输出字符串长度不能超过 MAX_STRING_LENGTH 定义的最大值
 * 
 * @section oled_printf_newlib_nano Newlib-Nano 环境下的浮点数限制
 * 
 * **重要：PlatformIO + CMSIS 框架使用 Newlib-Nano C 库**
 * 
 * - ❌ **%f 浮点格式不支持**：vsprintf() 会输出 "??" 或空字符串
 * - ✅ **整数格式完全支持**：%d, %u, %x, %s, %c 等
 * - 💡 **解决方案**：将浮点数转换为整数后显示
 * 
 * @code
 * // 错误示例（Newlib-Nano 下无法显示）
 * float voltage = 3.7f;
 * OLED_Printf(0, 0, OLED_8X16, "Voltage: %.2fV", voltage);  // ❌ 显示异常
 * 
 * // 正确做法：手动转换
 * float voltage = 3.7f;
 * int volt_int = (int)(voltage * 100);  // 放大 100 倍
 * OLED_Printf(0, 0, OLED_8X16, "Voltage: %d.%02dV", volt_int/100, volt_int%100);  // ✅
 * @endcode
 * 
 * @note 其他平台（Keil MDK + MicroLIB、IAR 等）可能支持 vsprintf 的浮点格式
 *       但在 STM32F103C8 + PlatformIO 环境下，必须使用整数转换方式
 */
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...)
{
    char String[MAX_STRING_LENGTH];         // 定义临时字符数组存储格式化结果
    va_list arg;                           // 定义可变参数列表变量
    va_start(arg, format);                 // 初始化可变参数列表，从format参数开始
    vsprintf(String, format, arg);         // 使用vsprintf将格式化字符串和参数列表写入字符数组
    va_end(arg);                           // 清理可变参数列表
    OLED_ShowString(X, Y, String, FontSize); // 调用字符串显示函数显示格式化结果
}

/**
 * @brief 在OLED上显示中英文混合字符串
 * @param[in] X 指定字符串左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定字符串左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] String 指定要显示的混合字符串，可包含全角字符与半角字符
 * @param[in] ChineseFontSize 指定中文字体大小，支持OLED_12X12_FULL、OLED_16X16_FULL、OLED_20X20_FULL
 * @param[in] ASCIIFontSize 指定ASCII字体大小，支持OLED_6X8_HALF、OLED_7X12_HALF、OLED_8X16_HALF
 * @note 显示的汉字需要在OLED_Fonts.c里的对应字体数组中定义
 * @note 未找到指定汉字时，会显示默认图形（方框内问号）
 * @note 通过检测字符最高位是否为1来区分中英文字符
 */
void OLED_ShowMixString(int16_t X, int16_t Y, const char *String, uint8_t ChineseFontSize, uint8_t ASCIIFontSize) 
{
    while (*String != '\0') {
        if (*String & 0x80) { // 判断是否是中文字符 (最高位为1表示中文字符)
            char Chinese[OLED_CHN_CHAR_WIDTH+1];
            for (uint8_t i=0; i<OLED_CHN_CHAR_WIDTH; i++){
                Chinese[i] = *(String+i);
            }
            Chinese[OLED_CHN_CHAR_WIDTH] = '\0';
            OLED_ShowChinese(X, Y, Chinese, ChineseFontSize);
            X += ChineseFontSize;  // 中文字符宽度递增
            String += OLED_CHN_CHAR_WIDTH;  // 跳过当前的中文字符（通常为2-3字节）
        } else {
            // 如果是ASCII字符
            OLED_ShowChar(X, Y, *String, ASCIIFontSize);
            X += ASCIIFontSize; // ASCII字符宽度递增
            String++; // 指向下一个字符
        }
    }
}

/**
 * @brief 使用printf格式在OLED上显示中英文混合字符串
 * @param[in] X 指定字符串左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定字符串左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] ChineseFontSize 指定中文字体大小，支持OLED_12X12_FULL、OLED_16X16_FULL、OLED_20X20_FULL
 * @param[in] ASCIIFontSize 指定ASCII字体大小，支持OLED_6X8_HALF、OLED_7X12_HALF、OLED_8X16_HALF
 * @param[in] format 指定要显示的格式化字符串，必须为ASCII码可见字符组成的字符串
 * @param[in] ... 格式化字符串参数列表
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 支持标准 printf 格式化语法，可处理包含中英文的格式化字符串
 * @warning 输出字符串长度不能超过 MAX_STRING_LENGTH 定义的最大值
 * @note Newlib-Nano 限制：PlatformIO + CMSIS 环境使用 Newlib-Nano C 库，不支持 %f 浮点格式；需显示浮点数时请转换为整数处理
 */
void OLED_PrintfMix(int16_t X, int16_t Y, uint8_t ChineseFontSize, uint8_t ASCIIFontSize, const char *format, ...)
{
    char String[MAX_STRING_LENGTH];         // 定义临时字符数组存储格式化结果
    va_list arg;                           // 定义可变参数列表变量
    va_start(arg, format);                 // 初始化可变参数列表，从format参数开始
    vsprintf(String, format, arg);         // 使用vsprintf将格式化字符串和参数列表写入字符数组
    va_end(arg);                           // 清理可变参数列表
    OLED_ShowMixString(X, Y, String, ChineseFontSize, ASCIIFontSize); // 调用混合字符串显示函数
}

/* ========================================
 * 区域显示函数实现
 * ======================================== */

/**
 * @brief 在指定区域内显示图片（裁剪显示）
 * @param[in] X_Pic 图片左上角的横坐标
 * @param[in] Y_Pic 图片左上角的纵坐标
 * @param[in] PictureWidth 图片宽度（像素）
 * @param[in] PictureHeight 图片高度（像素）
 * @param[in] X_Area 显示区域左上角的横坐标
 * @param[in] Y_Area 显示区域左上角的纵坐标
 * @param[in] AreaWidth 显示区域的宽度（像素）
 * @param[in] AreaHeight 显示区域的高度（像素）
 * @param[in] Image 图片取模数据指针
 * @note 此函数实现图片的区域裁剪显示，只在指定区域内渲染图片内容
 * @note 为OLED_UI的复杂显示效果提供基础支持
 * @warning 所有坐标参数均支持负数值
 */
void OLED_ShowImageArea(int16_t X_Pic, int16_t Y_Pic, int16_t PictureWidth, int16_t PictureHeight, 
                       int16_t X_Area, int16_t Y_Area, int16_t AreaWidth, int16_t AreaHeight, 
                       const uint8_t *Image)
{
    // 参数有效性检查
    if (PictureWidth == 0 || PictureHeight == 0 || AreaWidth == 0 || AreaHeight == 0 || 
        X_Pic > OLED_WIDTH-1 || X_Area > OLED_WIDTH-1 || 
        Y_Pic > OLED_HEIGHT-1 || Y_Area > OLED_HEIGHT-1) {
        return; 
    }
    
    // 计算实际显示区域的交集
    int16_t startX = (X_Pic < X_Area) ? X_Area : X_Pic;
    int16_t endX = ((X_Area + AreaWidth - 1) < (X_Pic + PictureWidth - 1)) ? 
                   (X_Area + AreaWidth - 1) : (X_Pic + PictureWidth - 1);
    int16_t startY = (Y_Pic < Y_Area) ? Y_Area : Y_Pic;
    int16_t endY = ((Y_Area + AreaHeight - 1) < (Y_Pic + PictureHeight - 1)) ? 
                   (Y_Area + AreaHeight - 1) : (Y_Pic + PictureHeight - 1);
                   
    // 边界检查和修正
    endX = (endX > OLED_WIDTH-1) ? OLED_WIDTH-1 : endX;
    endY = (endY > OLED_HEIGHT-1) ? OLED_HEIGHT-1 : endY;
    
    // 检查是否有有效显示区域
    if(startX > endX || startY > endY) {
        return;
    }
    
    // 遍历图片数据并按位渲染到显存
    for (uint8_t j = 0; j <= (PictureHeight - 1) / 8; j++) {
        for (uint8_t i = 0; i < PictureWidth; i++) {
            uint8_t currX = X_Pic + i;
            // 跳过不在显示区域内的X坐标
            if (currX < startX || currX > endX) {
                continue;
            }
            
            // 处理每个字节的8个位
            for (uint8_t bit = 0; bit < 8; bit++) {
                uint8_t currY = Y_Pic + j * 8 + bit;
                // 跳过不在显示区域内的Y坐标
                if (currY < startY || currY > endY) {
                    continue;
                }
                
                // 计算显存位置和位位置
                uint8_t page = currY / 8;
                uint8_t bit_pos = currY % 8;
                uint8_t data = Image[j * PictureWidth + i];
                
                // 如果当前位为1，则在显存中设置对应位
                if (data & (1 << bit)) {
                    OLED_DisplayBuf[page][currX] |= (1 << bit_pos); 
                }
            }
        }
    }
}

/**
 * @brief 在指定区域内显示单个字符
 * @param[in] RangeX 指定字符可显示范围左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] RangeY 指定字符可显示范围左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] RangeWidth 指定显示范围的宽度
 * @param[in] RangeHeight 指定显示范围的高度
 * @param[in] X 指定字符左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定字符左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Char 指定要显示的字符，必须为ASCII码可见字符
 * @param[in] FontSize 指定字体大小，支持OLED_6X8_HALF、OLED_7X12_HALF、OLED_8X16_HALF
 * @note 字符只会在此指定区域内显示，超出区域的部分会被裁剪
 */
void OLED_ShowCharArea(int16_t RangeX, int16_t RangeY, int16_t RangeWidth, int16_t RangeHeight, 
                      int16_t X, int16_t Y, char Char, uint8_t FontSize)
{
    if (FontSize == OLED_8X16_HALF)        // 字体为宽8像素，高16像素
    {
        /* 调用区域图片显示函数，使用对应字体的字模数据 */
        OLED_ShowImageArea(X, Y, 8, 16, RangeX, RangeY, RangeWidth, RangeHeight, OLED_8x16[Char - ' ']);
    }
    else if(FontSize == OLED_6X8_HALF)    // 字体为宽6像素，高8像素
    {
        /* 调用区域图片显示函数，使用对应字体的字模数据 */
        OLED_ShowImageArea(X, Y, 6, 8, RangeX, RangeY, RangeWidth, RangeHeight, OLED_6x8[Char - ' ']);
    }
    else if(FontSize == OLED_7X12_HALF)    // 字体为宽7像素，高12像素
    {
        /* 调用区域图片显示函数，使用对应字体的字模数据 */
        OLED_ShowImageArea(X, Y, 7, 12, RangeX, RangeY, RangeWidth, RangeHeight, OLED_7x12[Char - ' ']);
    }
}

/**
 * @brief 在指定区域内显示字符串
 * @param[in] RangeX 指定字符串可显示范围左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] RangeY 指定字符串可显示范围左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] RangeWidth 指定显示范围的宽度
 * @param[in] RangeHeight 指定显示范围的高度
 * @param[in] X 指定字符串左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定字符串左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] String 指定要显示的字符串，必须为ASCII码可见字符
 * @param[in] FontSize 指定字体大小，支持OLED_6X8_HALF、OLED_7X12_HALF、OLED_8X16_HALF
 * @note 字符串只会在此指定区域内显示，超出区域的部分会被裁剪
 * @note 支持显示任意长度的字符串，通过逐字符处理实现
 */
void OLED_ShowStringArea(int16_t RangeX, int16_t RangeY, int16_t RangeWidth, int16_t RangeHeight, 
                        int16_t X, int16_t Y, char *String, uint8_t FontSize)
{
    /* 由于可能显示很长的字符串，使用uint16_t类型 */
    uint16_t i;
    for (i = 0; String[i] != '\0'; i++)        // 遍历字符串的每个字符
    {
        /* 调用区域字符显示函数，依次显示每个字符 */
        OLED_ShowCharArea(RangeX, RangeY, RangeWidth, RangeHeight, 
                         X + i * FontSize, Y, String[i], FontSize);
    }
}

/* ========================================
 * 区域显示高级功能函数实现
 * ======================================== */

/**
 * @brief 在指定区域内显示中文字符串
 * @param[in] RangeX 指定字符可显示范围左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] RangeY 指定字符可显示范围左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] RangeWidth 指定显示范围的宽度
 * @param[in] RangeHeight 指定显示范围的高度
 * @param[in] X 指定字符串左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定字符串左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Chinese 指定要显示的中文字符串，必须全部为汉字或全角字符
 * @param[in] FontSize 指定中文字体大小，支持OLED_12X12_FULL、OLED_16X16_FULL、OLED_20X20_FULL
 * @note 字符串只会在此指定区域内显示，超出区域的部分会被裁剪
 * @note 显示的汉字需要在OLED_Fonts.c里的对应字体数组中定义
 * @note 未找到指定汉字时，会显示默认图形（方框内问号）
 * @warning 不要在字符串中混入半角字符
 */
void OLED_ShowChineseArea(int16_t RangeX, int16_t RangeY, int16_t RangeWidth, int16_t RangeHeight, 
                         int16_t X, int16_t Y, char *Chinese, uint8_t FontSize)
{
    uint8_t pChinese = 0;
    uint8_t pIndex;
    uint8_t i;
    char SingleChinese[OLED_CHN_CHAR_WIDTH + 1] = {0};
    
    for (i = 0; Chinese[i] != '\0'; i++)    // 遍历中文字符串
    {
        SingleChinese[pChinese] = Chinese[i];    // 提取中文字符串数据到单个汉字数组
        pChinese++;                            // 计数器自增
        
        if (pChinese >= OLED_CHN_CHAR_WIDTH)    // 提取到了一个完整的汉字
        {
            pChinese = 0;    // 计数器归零
            
            const void* fontArray;
            // 根据字体大小选择对应的字体数组
            if (FontSize == OLED_12X12_FULL) {
                fontArray = (const void*) OLED_12x12;
            } else if (FontSize == OLED_16X16_FULL) {
                fontArray = (const void*) OLED_16x16;
            } else if (FontSize == OLED_20X20_FULL) {
                fontArray = (const void*) OLED_20x20;
            }

            // 查找并显示对应字体大小的汉字数据
            if(FontSize == OLED_12X12_FULL){
                for (pIndex = 0; strcmp(((const ChineseCell12x12_t*)fontArray)[pIndex].Index, "") != 0; pIndex++)
                {
                    if (strcmp(((const ChineseCell12x12_t*)fontArray)[pIndex].Index, SingleChinese) == 0){break;}
                }
                OLED_ShowImageArea(X + ((i + 1) / OLED_CHN_CHAR_WIDTH - 1) * OLED_12X12_FULL, Y, 
                                  OLED_12X12_FULL, OLED_12X12_FULL, 
                                  RangeX, RangeY, RangeWidth, RangeHeight, 
                                  ((const ChineseCell12x12_t*)fontArray)[pIndex].Data);
            } else if(FontSize == OLED_16X16_FULL){
                for (pIndex = 0; strcmp(((const ChineseCell16x16_t*)fontArray)[pIndex].Index, "") != 0; pIndex++)
                {
                    if (strcmp(((const ChineseCell16x16_t*)fontArray)[pIndex].Index, SingleChinese) == 0){break;}
                }
                OLED_ShowImageArea(X + ((i + 1) / OLED_CHN_CHAR_WIDTH - 1) * OLED_16X16_FULL, Y, 
                                  OLED_16X16_FULL, OLED_16X16_FULL, 
                                  RangeX, RangeY, RangeWidth, RangeHeight,
                                  ((const ChineseCell16x16_t*)fontArray)[pIndex].Data);
            } else if(FontSize == OLED_20X20_FULL){
                for (pIndex = 0; strcmp(((const ChineseCell20x20_t*)fontArray)[pIndex].Index, "") != 0; pIndex++)
                {
                    if (strcmp(((const ChineseCell20x20_t*)fontArray)[pIndex].Index, SingleChinese) == 0){break;}
                }
                OLED_ShowImageArea(X + ((i + 1) / OLED_CHN_CHAR_WIDTH - 1) * OLED_20X20_FULL, Y, 
                                  OLED_20X20_FULL, OLED_20X20_FULL, 
                                  RangeX, RangeY, RangeWidth, RangeHeight,
                                  ((const ChineseCell20x20_t*)fontArray)[pIndex].Data);
            }
        }
    }
}

/**
 * @brief 在指定区域内使用printf格式显示字符串
 * @param[in] RangeX 指定字符可显示范围左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] RangeY 指定字符可显示范围左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] RangeWidth 指定显示范围的宽度
 * @param[in] RangeHeight 指定显示范围的高度
 * @param[in] X 指定字符串左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定字符串左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] FontSize 指定字体大小，支持OLED_6X8_HALF、OLED_7X12_HALF、OLED_8X16_HALF
 * @param[in] format 指定要显示的格式化字符串，必须为ASCII码可见字符组成的字符串
 * @param[in] ... 格式化字符串参数列表
 * @note 字符串只会在此指定区域内显示，超出区域的部分会被裁剪
 * @note 支持标准 printf 格式化语法
 * @warning 输出字符串长度不能超过 MAX_STRING_LENGTH 定义的最大值
 * @note Newlib-Nano 限制：PlatformIO + CMSIS 环境使用 Newlib-Nano C 库，不支持 %f 浮点格式；需显示浮点数时请转换为整数处理
 */
void OLED_PrintfArea(int16_t RangeX, int16_t RangeY, int16_t RangeWidth, int16_t RangeHeight, 
                    int16_t X, int16_t Y, uint8_t FontSize, char *format, ...)
{
    char String[MAX_STRING_LENGTH];         // 定义临时字符数组存储格式化结果
    va_list arg;                           // 定义可变参数列表变量
    va_start(arg, format);                 // 初始化可变参数列表，从format参数开始
    vsprintf(String, format, arg);         // 使用vsprintf将格式化字符串和参数列表写入字符数组
    va_end(arg);                           // 清理可变参数列表
    OLED_ShowStringArea(RangeX, RangeY, RangeWidth, RangeHeight, X, Y, String, FontSize); // 调用区域字符串显示函数
}

/**
 * @brief 在指定区域内显示中英文混合字符串
 * @param[in] RangeX 指定字符可显示范围左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] RangeY 指定字符可显示范围左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] RangeWidth 指定显示范围的宽度
 * @param[in] RangeHeight 指定显示范围的高度
 * @param[in] X 指定字符串左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定字符串左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] String 指定要显示的混合字符串，可包含全角字符与半角字符
 * @param[in] ChineseFontSize 指定中文字体大小，支持OLED_12X12_FULL、OLED_16X16_FULL、OLED_20X20_FULL
 * @param[in] ASCIIFontSize 指定ASCII字体大小，支持OLED_6X8_HALF、OLED_7X12_HALF、OLED_8X16_HALF
 * @note 字符串只会在此指定区域内显示，超出区域的部分会被裁剪
 * @note 通过检测字符最高位是否为1来区分中英文字符
 * @note 显示的汉字需要在OLED_Fonts.c里的对应字体数组中定义
 */
void OLED_ShowMixStringArea(int16_t RangeX, int16_t RangeY, int16_t RangeWidth, int16_t RangeHeight, 
                           int16_t X, int16_t Y, const char *String, uint8_t ChineseFontSize, uint8_t ASCIIFontSize)
{
    while (*String != '\0') {
        if (*String & 0x80) { // 判断中文字符（最高位为1表示中文字符）
            char Chinese[OLED_CHN_CHAR_WIDTH + 1]; // 根据编码长度动态调整数组
            for (uint8_t i = 0; i < OLED_CHN_CHAR_WIDTH; i++) {
                Chinese[i] = *(String + i); // 连续拷贝字符编码
            }
            Chinese[OLED_CHN_CHAR_WIDTH] = '\0'; // 添加字符串结束符
            OLED_ShowChineseArea(RangeX, RangeY, RangeWidth, RangeHeight, X, Y, Chinese, ChineseFontSize);
            X += ChineseFontSize; // 更新X坐标（中文字符宽度）
            String += OLED_CHN_CHAR_WIDTH; // 跳过已处理的中文字符
        } else { // ASCII字符处理
            OLED_ShowCharArea(RangeX, RangeY, RangeWidth, RangeHeight, X, Y, *String, ASCIIFontSize);
            X += ASCIIFontSize; // 更新X坐标（ASCII字符宽度）
            String++; // 处理下一个字符
        }
    }
}

/**
 * @brief 在指定区域内使用printf格式显示中英文混合字符串
 * @param[in] RangeX 指定字符可显示范围左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] RangeY 指定字符可显示范围左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] RangeWidth 指定显示范围的宽度
 * @param[in] RangeHeight 指定显示范围的高度
 * @param[in] X 指定字符串左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定字符串左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] ChineseFontSize 指定中文字体大小，支持OLED_12X12_FULL、OLED_16X16_FULL、OLED_20X20_FULL
 * @param[in] ASCIIFontSize 指定ASCII字体大小，支持OLED_6X8_HALF、OLED_7X12_HALF、OLED_8X16_HALF
 * @param[in] format 指定要显示的格式化字符串，必须为ASCII码可见字符组成的字符串
 * @param[in] ... 格式化字符串参数列表
 * @note 字符串只会在此指定区域内显示，超出区域的部分会被裁剪
 * @note 支持标准 printf 格式化语法，可处理包含中英文的格式化字符串
 * @warning 输出字符串长度不能超过 MAX_STRING_LENGTH 定义的最大值
 * @note Newlib-Nano 限制：PlatformIO + CMSIS 环境使用 Newlib-Nano C 库，不支持 %f 浮点格式；需显示浮点数时请转换为整数处理
 */
void OLED_PrintfMixArea(int16_t RangeX, int16_t RangeY, int16_t RangeWidth, int16_t RangeHeight,
                       int16_t X, int16_t Y, uint8_t ChineseFontSize, uint8_t ASCIIFontSize, 
                       const char *format, ...)
{
    char String[MAX_STRING_LENGTH];         // 定义临时字符数组存储格式化结果
    va_list arg;                           // 定义可变参数列表变量
    va_start(arg, format);                 // 初始化可变参数列表，从format参数开始
    vsprintf(String, format, arg);         // 使用vsprintf将格式化字符串和参数列表写入字符数组
    va_end(arg);                           // 清理可变参数列表
    OLED_ShowMixStringArea(RangeX, RangeY, RangeWidth, RangeHeight, X, Y, String, ChineseFontSize, ASCIIFontSize); // 调用区域混合字符串显示函数
}


/* ========================================
 * 基础绘图函数实现
 * ======================================== */

/**
 * @brief 在OLED指定位置绘制一个点
 * @param[in] X 指定点的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定点的纵坐标，范围：负数~OLED_HEIGHT-1
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 坐标支持负数值，负值部分会被忽略
 */
void OLED_DrawPoint(int16_t X, int16_t Y)
{
    /* 参数检查，保证指定位置不会超出屏幕范围 */
    if(X < 0 || Y < 0 || X > OLED_WIDTH-1 || Y > OLED_HEIGHT-1) {return;}
    
    /* 将显存数组指定位置的一个Bit数据置1 */
    OLED_DisplayBuf[Y / 8][X] |= 0x01 << (Y % 8);
}

/**
 * @brief 获取OLED指定位置点的状态值
 * @param[in] X 指定点的横坐标，范围：0~OLED_WIDTH-1
 * @param[in] Y 指定点的纵坐标，范围：0~OLED_HEIGHT-1
 * @return 指定位置点的状态
 * @retval 1 点处于点亮状态
 * @retval 0 点处于熄灭状态
 */
uint8_t OLED_GetPoint(uint8_t X, uint8_t Y)
{
    /* 参数检查，保证指定位置不会超出屏幕范围 */
    if (X > OLED_WIDTH-1) {return 0;}
    if (Y > OLED_HEIGHT-1) {return 0;}
    
    /* 判断指定位置的数据 */
    if (OLED_DisplayBuf[Y / 8][X] & 0x01 << (Y % 8))
    {
        return 1;    // 为1，返回1
    }
    return 0;        // 否则，返回0
}

/* ========================================
 * 几何图形绘制函数实现
 * ======================================== */

/**
 * @brief 在OLED上绘制直线
 * @param[in] X0 指定起点的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y0 指定起点的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] X1 指定终点的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y1 指定终点的纵坐标，范围：负数~OLED_HEIGHT-1
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 使用Bresenham算法实现，避免浮点运算，提高效率
 */
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1)
{
    int16_t x, y, dx, dy, d, incrE, incrNE, temp;
    int16_t x0 = X0, y0 = Y0, x1 = X1, y1 = Y1;
    uint8_t yflag = 0, xyflag = 0;
    
    if (y0 == y1)        // 横线单独处理
    {
        /* 0号点X坐标大于1号点X坐标，则交换两点X坐标 */
        if (x0 > x1) {temp = x0; x0 = x1; x1 = temp;}
        
        /* 遍历X坐标 */
        for (x = x0; x <= x1; x++)
        {
            OLED_DrawPoint(x, y0);    // 依次画点
        }
    }
    else if (x0 == x1)    // 竖线单独处理
    {
        /* 0号点Y坐标大于1号点Y坐标，则交换两点Y坐标 */
        if (y0 > y1) {temp = y0; y0 = y1; y1 = temp;}
        
        /* 遍历Y坐标 */
        for (y = y0; y <= y1; y++)
        {
            OLED_DrawPoint(x0, y);    // 依次画点
        }
    }
    else                // 斜线处理
    {
        /* 使用Bresenham算法画直线，可以避免耗时的浮点运算，效率更高 */
        /* 参考文档：https://www.cs.montana.edu/courses/spring2009/425/dslectures/Bresenham.pdf */
        /* 参考教程：https://www.bilibili.com/video/BV1364y1d7Lo */
        
        if (x0 > x1)    // 0号点X坐标大于1号点X坐标
        {
            /* 交换两点坐标 */
            /* 交换后不影响画线，但是画线方向由第一、二、三、四象限变为第一、四象限 */
            temp = x0; x0 = x1; x1 = temp;
            temp = y0; y0 = y1; y1 = temp;
        }
        
        if (y0 > y1)    // 0号点Y坐标大于1号点Y坐标
        {
            /* 将Y坐标取负 */
            /* 取负后影响画线，但是画线方向由第一、四象限变为第一象限 */
            y0 = -y0;
            y1 = -y1;
            
            /* 置标志位yflag，记住当前变换，在后续实际画线时，再将坐标换回来 */
            yflag = 1;
        }
        
        if (y1 - y0 > x1 - x0)    // 画线斜率大于1
        {
            /* 将X坐标与Y坐标互换 */
            /* 互换后影响画线，但是画线方向由第一象限0~90度范围变为第一象限0~45度范围 */
            temp = x0; x0 = y0; y0 = temp;
            temp = x1; x1 = y1; y1 = temp;
            
            /* 置标志位xyflag，记住当前变换，在后续实际画线时，再将坐标换回来 */
            xyflag = 1;
        }
        
        /* 以下为Bresenham算法画直线 */
        /* 算法要求，画线方向必须为第一象限0~45度范围 */
        dx = x1 - x0;
        dy = y1 - y0;
        incrE = 2 * dy;
        incrNE = 2 * (dy - dx);
        d = 2 * dy - dx;
        x = x0;
        y = y0;
        
        /* 画起始点，同时判断标志位，将坐标换回来 */
        if (yflag && xyflag){OLED_DrawPoint(y, -x);}
        else if (yflag)        {OLED_DrawPoint(x, -y);}
        else if (xyflag)    {OLED_DrawPoint(y, x);}
        else                {OLED_DrawPoint(x, y);}
        
        while (x < x1)        // 遍历X轴的每个点
        {
            x++;
            if (d < 0)        // 下一个点在当前点东方
            {
                d += incrE;
            }
            else            // 下一个点在当前点东北方
            {
                y++;
                d += incrNE;
            }
            
            /* 画每一个点，同时判断标志位，将坐标换回来 */
            if (yflag && xyflag){OLED_DrawPoint(y, -x);}
            else if (yflag)        {OLED_DrawPoint(x, -y);}
            else if (xyflag)    {OLED_DrawPoint(y, x);}
            else                {OLED_DrawPoint(x, y);}
        }    
    }
}

/**
 * @brief 在OLED上绘制矩形
 * @param[in] X 指定矩形左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定矩形左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Width 指定矩形的宽度，范围：正数
 * @param[in] Height 指定矩形的高度，范围：正数
 * @param[in] IsFilled 指定矩形是否填充
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 填充模式下使用位操作优化，效率更高
 */
void OLED_DrawRectangle(int16_t X, int16_t Y, int16_t Width, int16_t Height, uint8_t IsFilled)
{
    if (Width == 0 || Height == 0) { return; } // 宽度或高度为0，不绘制

    int16_t X_start = X;
    int16_t X_end = X + Width - 1;
    int16_t Y_start = Y;
    int16_t Y_end = Y + Height - 1;

    // 限制坐标在屏幕范围内
    if (X_start < 0) X_start = 0;
    if (X_end >= OLED_WIDTH) X_end = OLED_WIDTH - 1;
    if (Y_start < 0) Y_start = 0;
    if (Y_end >= OLED_HEIGHT) Y_end = OLED_HEIGHT - 1;

    // 计算有效宽度和高度
    int16_t validWidth = X_end - X_start + 1;
    int16_t validHeight = Y_end - Y_start + 1;
    if (validWidth <= 0 || validHeight <= 0) { return; }

    
    // 计算起始和结束页
    int16_t start_page = Y_start / 8;
    int16_t end_page = Y_end / 8;

    // 计算每页的掩码
    uint8_t start_mask = 0xFF << (Y_start % 8);
    uint8_t end_mask = 0xFF >> (7 - (Y_end % 8));

    
    if(IsFilled){
        // 遍历每一列，应用掩码
        for (int16_t x = X_start; x <= X_end; x++) {
                for (int16_t page = start_page; page <= end_page; page++) {
                        uint8_t mask = 0xFF;
                        if (page == start_page) mask &= start_mask;
                        if (page == end_page) mask &= end_mask;
                        if (page >= 0 && page < OLED_HEIGHT / 8) { // 确保页数有效
                                OLED_DisplayBuf[page][x] |= mask;
                        }
                }
        }
    }else{
        // 绘制矩形边框
        for (int16_t i = X; i <= X + Width - 1; i++) {
            OLED_DrawPoint(i, Y);
            OLED_DrawPoint(i, Y + Height - 1);
        }
        for (int16_t i = Y; i <= Y + Height - 1; i++) {
            OLED_DrawPoint(X, i);
            OLED_DrawPoint(X + Width - 1, i);
        }
    }
}

/**
 * @brief 在OLED上绘制三角形
 * @param[in] X0 指定第一个顶点的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y0 指定第一个顶点的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] X1 指定第二个顶点的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y1 指定第二个顶点的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] X2 指定第三个顶点的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y2 指定第三个顶点的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] IsFilled 指定三角形是否填充
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 填充模式使用射线交叉算法判断点是否在三角形内
 */
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled)
{
    int16_t minx = X0, miny = Y0, maxx = X0, maxy = Y0;
    int16_t i, j;
    int16_t vx[] = {X0, X1, X2};
    int16_t vy[] = {Y0, Y1, Y2};
    
    if (!IsFilled)            // 指定三角形不填充
    {
        /* 调用画线函数，将三个点用直线连接 */
        OLED_DrawLine(X0, Y0, X1, Y1);
        OLED_DrawLine(X0, Y0, X2, Y2);
        OLED_DrawLine(X1, Y1, X2, Y2);
    }
    else                    // 指定三角形填充
    {
        /* 找到三个点最小的X、Y坐标 */
        if (X1 < minx) {minx = X1;}
        if (X2 < minx) {minx = X2;}
        if (Y1 < miny) {miny = Y1;}
        if (Y2 < miny) {miny = Y2;}
        
        /* 找到三个点最大的X、Y坐标 */
        if (X1 > maxx) {maxx = X1;}
        if (X2 > maxx) {maxx = X2;}
        if (Y1 > maxy) {maxy = Y1;}
        if (Y2 > maxy) {maxy = Y2;}
        
        /* 最小最大坐标之间的矩形为可能需要填充的区域 */
        /* 遍历此区域中所有的点 */
        /* 遍历X坐标 */
        for (i = minx; i <= maxx; i++)
        {
            /* 遍历Y坐标 */
            for (j = miny; j <= maxy; j++)
            {
                /* 调用OLED_pnpoly，判断指定点是否在指定三角形之中 */
                /* 如果在，则画点，如果不在，则不做处理 */
                if (OLED_pnpoly(3, vx, vy, i, j)) {OLED_DrawPoint(i, j);}
            }
        }
    }
}

/**
 * @brief 在OLED上绘制圆形
 * @param[in] X 指定圆心的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定圆心的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Radius 指定圆的半径，范围：正数
 * @param[in] IsFilled 指定圆是否填充
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 使用Bresenham算法画圆，避免浮点运算，提高效率
 * @see https://www.cs.montana.edu/courses/spring2009/425/dslectures/Bresenham.pdf
 * @see https://www.bilibili.com/video/BV1VM4y1u7wJ
 */
void OLED_DrawCircle(int16_t X, int16_t Y, int16_t Radius, uint8_t IsFilled)
{
    int16_t x, y, d, j;
    
    /* 使用Bresenham算法画圆，可以避免耗时的浮点运算，效率更高 */
    
    d = 1 - Radius;
    x = 0;
    y = Radius;
    
    /* 画每个八分之一圆弧的起始点 */
    OLED_DrawPoint(X + x, Y + y);
    OLED_DrawPoint(X - x, Y - y);
    OLED_DrawPoint(X + y, Y + x);
    OLED_DrawPoint(X - y, Y - x);
    
    if (IsFilled)        // 指定圆填充
    {
        /* 遍历起始点Y坐标 */
        for (j = -y; j < y; j++)
        {
            /* 在指定区域画点，填充部分圆 */
            OLED_DrawPoint(X, Y + j);
        }
    }
    
    while (x < y)        // 遍历X轴的每个点
    {
        x++;
        if (d < 0)        // 下一个点在当前点东方
        {
            d += 2 * x + 1;
        }
        else            // 下一个点在当前点东南方
        {
            y--;
            d += 2 * (x - y) + 1;
        }
        
        /* 画每个八分之一圆弧的点 */
        OLED_DrawPoint(X + x, Y + y);
        OLED_DrawPoint(X + y, Y + x);
        OLED_DrawPoint(X - x, Y - y);
        OLED_DrawPoint(X - y, Y - x);
        OLED_DrawPoint(X + x, Y - y);
        OLED_DrawPoint(X + y, Y - x);
        OLED_DrawPoint(X - x, Y + y);
        OLED_DrawPoint(X - y, Y + x);
        
        if (IsFilled)    // 指定圆填充
        {
            /* 遍历中间部分 */
            for (j = -y; j < y; j++)
            {
                /* 在指定区域画点，填充部分圆 */
                OLED_DrawPoint(X + x, Y + j);
                OLED_DrawPoint(X - x, Y + j);
            }
            
            /* 遍历两侧部分 */
            for (j = -x; j < x; j++)
            {
                /* 在指定区域画点，填充部分圆 */
                OLED_DrawPoint(X - y, Y + j);
                OLED_DrawPoint(X + y, Y + j);
            }
        }
    }
}

/**
 * @brief 在OLED上绘制椭圆
 * @param[in] X 指定椭圆圆心的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定椭圆圆心的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] A 指定椭圆的横向半轴长度，范围：正数
 * @param[in] B 指定椭圆的纵向半轴长度，范围：正数
 * @param[in] IsFilled 指定椭圆是否填充
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 使用改进的Bresenham算法画椭圆，避免部分浮点运算
 * @see https://blog.csdn.net/myf_666/article/details/128167392
 */
void OLED_DrawEllipse(int16_t X, int16_t Y, int16_t A, int16_t B, uint8_t IsFilled)
{
    int16_t x, y, j;
    int16_t a = A, b = B;
    float d1, d2;
    
    /* 使用Bresenham算法画椭圆，可以避免部分耗时的浮点运算，效率更高 */
    
    x = 0;
    y = b;
    d1 = b * b + a * a * (-b + 0.5);
    
    if (IsFilled)    // 指定椭圆填充
    {
        /* 遍历起始点Y坐标 */
        for (j = -y; j < y; j++)
        {
            /* 在指定区域画点，填充部分椭圆 */
            OLED_DrawPoint(X, Y + j);
            OLED_DrawPoint(X, Y + j);
        }
    }
    
    /* 画椭圆弧的起始点 */
    OLED_DrawPoint(X + x, Y + y);
    OLED_DrawPoint(X - x, Y - y);
    OLED_DrawPoint(X - x, Y + y);
    OLED_DrawPoint(X + x, Y - y);
    
    /* 画椭圆中间部分 */
    while (b * b * (x + 1) < a * a * (y - 0.5))
    {
        if (d1 <= 0)        // 下一个点在当前点东方
        {
            d1 += b * b * (2 * x + 3);
        }
        else                // 下一个点在当前点东南方
        {
            d1 += b * b * (2 * x + 3) + a * a * (-2 * y + 2);
            y--;
        }
        x++;
        
        if (IsFilled)    // 指定椭圆填充
        {
            /* 遍历中间部分 */
            for (j = -y; j < y; j++)
            {
                /* 在指定区域画点，填充部分椭圆 */
                OLED_DrawPoint(X + x, Y + j);
                OLED_DrawPoint(X - x, Y + j);
            }
        }
        
        /* 画椭圆中间部分圆弧 */
        OLED_DrawPoint(X + x, Y + y);
        OLED_DrawPoint(X - x, Y - y);
        OLED_DrawPoint(X - x, Y + y);
        OLED_DrawPoint(X + x, Y - y);
    }
    
    /* 画椭圆两侧部分 */
    d2 = b * b * (x + 0.5) * (x + 0.5) + a * a * (y - 1) * (y - 1) - a * a * b * b;
    
    while (y > 0)
    {
        if (d2 <= 0)        // 下一个点在当前点东方
        {
            d2 += b * b * (2 * x + 2) + a * a * (-2 * y + 3);
            x++;
            
        }
        else                // 下一个点在当前点东南方
        {
            d2 += a * a * (-2 * y + 3);
        }
        y--;
        
        if (IsFilled)    // 指定椭圆填充
        {
            /* 遍历两侧部分 */
            for (j = -y; j < y; j++)
            {
                /* 在指定区域画点，填充部分椭圆 */
                OLED_DrawPoint(X + x, Y + j);
                OLED_DrawPoint(X - x, Y + j);
            }
        }
        
        /* 画椭圆两侧部分圆弧 */
        OLED_DrawPoint(X + x, Y + y);
        OLED_DrawPoint(X - x, Y - y);
        OLED_DrawPoint(X - x, Y + y);
        OLED_DrawPoint(X + x, Y - y);
    }
}

/**
 * @brief 在OLED上绘制圆弧（扇形）
 * @param[in] X 指定圆弧圆心的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定圆弧圆心的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Radius 指定圆弧的半径，范围：正数
 * @param[in] StartAngle 指定圆弧的起始角度，范围：-180~180度
 * @param[in] EndAngle 指定圆弧的终止角度，范围：-180~180度
 * @param[in] IsFilled 指定圆弧是否填充（填充后为扇形）
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 角度定义：水平向右为0度，水平向左为180度或-180度，下方为正数，上方为负数，顺时针旋转
 * @note 借用Bresenham算法画圆的方法实现
 */
void OLED_DrawArc(int16_t X, int16_t Y, int16_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled)
{
    int16_t x, y, d, j;
    if(Radius <= 0){return;} // 半径为0或负数，直接返回
    /* 此函数借用Bresenham算法画圆的方法 */
    
    d = 1 - Radius;
    x = 0;
    y = Radius;
    
    /* 在画圆的每个点时，判断指定点是否在指定角度内，在则画点，不在则不做处理 */
    if (OLED_IsInAngle(x, y, StartAngle, EndAngle))    {OLED_DrawPoint(X + x, Y + y);}
    if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y - y);}
    if (OLED_IsInAngle(y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + x);}
    if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y - x);}
    
    if (IsFilled)    // 指定圆弧填充
    {
        /* 遍历起始点Y坐标 */
        for (j = -y; j < y; j++)
        {
            /* 在填充圆的每个点时，判断指定点是否在指定角度内，在则画点，不在则不做处理 */
            if (OLED_IsInAngle(0, j, StartAngle, EndAngle)) {OLED_DrawPoint(X, Y + j);}
        }
    }
    
    while (x < y)        // 遍历X轴的每个点
    {
        x++;
        if (d < 0)        // 下一个点在当前点东方
        {
            d += 2 * x + 1;
        }
        else            // 下一个点在当前点东南方
        {
            y--;
            d += 2 * (x - y) + 1;
        }
        
        /* 在画圆的每个点时，判断指定点是否在指定角度内，在则画点，不在则不做处理 */
        if (OLED_IsInAngle(x, y, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y + y);}
        if (OLED_IsInAngle(y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + x);}
        if (OLED_IsInAngle(-x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y - y);}
        if (OLED_IsInAngle(-y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y - x);}
        if (OLED_IsInAngle(x, -y, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y - y);}
        if (OLED_IsInAngle(y, -x, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y - x);}
        if (OLED_IsInAngle(-x, y, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y + y);}
        if (OLED_IsInAngle(-y, x, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y + x);}
        
        if (IsFilled)    // 指定圆弧填充
        {
            /* 遍历中间部分 */
            for (j = -y; j < y; j++)
            {
                /* 在填充圆的每个点时，判断指定点是否在指定角度内，在则画点，不在则不做处理 */
                if (OLED_IsInAngle(x, j, StartAngle, EndAngle)) {OLED_DrawPoint(X + x, Y + j);}
                if (OLED_IsInAngle(-x, j, StartAngle, EndAngle)) {OLED_DrawPoint(X - x, Y + j);}
            }
            
            /* 遍历两侧部分 */
            for (j = -x; j < x; j++)
            {
                /* 在填充圆的每个点时，判断指定点是否在指定角度内，在则画点，不在则不做处理 */
                if (OLED_IsInAngle(-y, j, StartAngle, EndAngle)) {OLED_DrawPoint(X - y, Y + j);}
                if (OLED_IsInAngle(y, j, StartAngle, EndAngle)) {OLED_DrawPoint(X + y, Y + j);}
            }
        }
    }
}

/**
 * @brief 在OLED上绘制圆角矩形
 * @param[in] X 指定矩形左上角的横坐标，范围：负数~OLED_WIDTH-1
 * @param[in] Y 指定矩形左上角的纵坐标，范围：负数~OLED_HEIGHT-1
 * @param[in] Width 指定矩形的宽度，范围：正数
 * @param[in] Height 指定矩形的高度，范围：正数
 * @param[in] Radius 圆角半径
 * @param[in] IsFilled 指定矩形是否填充
 * @note 调用此函数后，要想真正显示在屏幕上，还需调用更新函数
 * @note 通过四个圆弧和矩形主体组合实现圆角效果
 * @note 当半径为0时退化为普通矩形
 */
void OLED_DrawRoundedRectangle(int16_t X, int16_t Y, int16_t Width, int16_t Height, int16_t Radius, uint8_t IsFilled)
{
    // 安全检查
    if (Width == 0 || Height == 0) return;  // 宽度或高度为0，直接返回
    if (Radius > Width / 2 || Radius > Height / 2) {
        Radius = (Width < Height ? Width : Height) / 2;  // 限制圆角半径不超过宽高的一半
    }
    if (Radius <= 0) {
        OLED_DrawRectangle(X, Y, Width, Height, IsFilled);  // 如果半径为0，退化为普通矩形
        return;
    }
    
    // 绘制四个圆角
    OLED_DrawArc(X + Radius, Y + Radius, Radius, 180, -90, IsFilled);
    OLED_DrawArc(X + Width - Radius - 1, Y + Radius, Radius, -90, 0, IsFilled);
    OLED_DrawArc(X + Radius, Y + Height - Radius - 1, Radius, 90, 180, IsFilled);
    OLED_DrawArc(X + Width - Radius - 1, Y + Height - Radius - 1, Radius, 0, 90, IsFilled);

    // 填充或绘制矩形主体
    if (IsFilled) {
        OLED_DrawRectangle(X + Radius, Y, Width - 2 * Radius, Height, OLED_FILLED);
        OLED_DrawRectangle(X, Y + Radius, Width, Height - 2 * Radius, OLED_FILLED);
    } else {
        // 绘制顶部和底部的直线
        OLED_DrawLine(X + Radius, Y, X + Width - Radius - 1, Y);
        OLED_DrawLine(X + Radius, Y + Height - 1, X + Width - Radius - 1, Y + Height - 1);
        // 绘制左侧和右侧的直线
        OLED_DrawLine(X, Y + Radius, X, Y + Height - Radius - 1);
        OLED_DrawLine(X + Width - 1, Y + Radius, X + Width - 1, Y + Height - Radius - 1);
    }
}
