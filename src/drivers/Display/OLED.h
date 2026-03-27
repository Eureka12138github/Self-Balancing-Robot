/**
 * @file OLED.h
 * @brief OLED显示驱动软件层头文件
 * @author 江协科技 & 上nm网课呢
 * @date 2026-02-22
 */

#ifndef __OLED_H
#define __OLED_H

#ifdef __cplusplus
extern "C" {
#endif
#include "stm32f10x.h"                  // Device header
#include "stdbool.h"
	
//断言	
#ifdef OLED_ENABLE_ASSERTIONS
    void my_assert_handler(const char* expr, const char* file, int line);
    #define OLED_ASSERT(expr) do { \
        if (!(expr)) my_assert_handler(#expr, __FILE__, __LINE__); \
    } while(0)
#else
    #define OLED_ASSERT(expr) ((void)0)
#endif	
	

// 字体大小定义
#define OLED_6X8_HALF       (6)     // ASCII字符 6x8
#define OLED_8X16_HALF      (8)     // ASCII字符 8x16
#define OLED_7X12_HALF      (7)     // ASCII字符 7x12

#define OLED_8X8_FULL       (8)     // 中文字符 8x8
#define OLED_12X12_FULL     (12)    // 中文字符 12x12
#define OLED_16X16_FULL     (16)    // 中文字符 16x16
#define OLED_20X20_FULL     (20)    // 中文字符 20x20

// 填充模式定义
#define OLED_UNFILLED       (0)
#define OLED_FILLED         (1)

// 字符串最大长度
#define MAX_STRING_LENGTH   (128)

	

/*============================================================================
 *                          对外接口函数声明
 *============================================================================*/


// I2C通信函数 
void OLED_WriteCommand(uint8_t Command);
void OLED_WriteData(uint8_t *Data, uint8_t Count);

// OLED 初始化函数 
void OLED_Init(void);	

// 显示控制函数
void OLED_Update(void);
void OLED_UpdateArea(uint8_t X, uint8_t Y, uint8_t Width, uint8_t Height);
void OLED_Clear(void);
void OLED_ClearArea(int16_t X, int16_t Y, int16_t Width, int16_t Height);
void OLED_Reverse(void);
void OLED_ReverseArea(int16_t X, int16_t Y, int16_t Width, int16_t Height);

// 图像显示函数
void OLED_ShowImage(int16_t X, int16_t Y, uint16_t Width, uint16_t Height, const uint8_t *Image);

// 文本显示函数
void OLED_ShowChar(int16_t X, int16_t Y, char Char, uint8_t FontSize);
void OLED_ShowNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowSignedNum(int16_t X, int16_t Y, int32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowHexNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowBinNum(int16_t X, int16_t Y, uint32_t Number, uint8_t Length, uint8_t FontSize);
void OLED_ShowFloatNum(int16_t X, int16_t Y, double Number, uint8_t IntLength, uint8_t FraLength, uint8_t FontSize);
void OLED_ShowString(int16_t X, int16_t Y, char *String, uint8_t FontSize);
void OLED_ShowMixString(int16_t X, int16_t Y, const char *String, uint8_t ChineseFontSize, uint8_t ASCIIFontSize);
void OLED_ShowChinese(int16_t X, int16_t Y, const char *Chinese, uint8_t FontSize);
void OLED_Printf(int16_t X, int16_t Y, uint8_t FontSize, char *format, ...);
void OLED_PrintfMix(int16_t X, int16_t Y, uint8_t ChineseFontSize, uint8_t ASCIIFontSize, const char *format, ...);

// 区域显示函数
void OLED_ShowImageArea(int16_t X_Pic, int16_t Y_Pic, int16_t PictureWidth, int16_t PictureHeight, 
                       int16_t X_Area, int16_t Y_Area, int16_t AreaWidth, int16_t AreaHeight, const uint8_t *Image);
void OLED_ShowCharArea(int16_t RangeX, int16_t RangeY, int16_t RangeWidth, int16_t RangeHeight, 
                      int16_t X, int16_t Y, char Char, uint8_t FontSize);
void OLED_ShowStringArea(int16_t RangeX, int16_t RangeY, int16_t RangeWidth, int16_t RangeHeight, 
                        int16_t X, int16_t Y, char *String, uint8_t FontSize);
void OLED_ShowChineseArea(int16_t RangeX, int16_t RangeY, int16_t RangeWidth, int16_t RangeHeight, 
                         int16_t X, int16_t Y, char *Chinese, uint8_t FontSize);
void OLED_PrintfArea(int16_t RangeX, int16_t RangeY, int16_t RangeWidth, int16_t RangeHeight, 
                    int16_t X, int16_t Y, uint8_t FontSize, char *format, ...);
void OLED_ShowMixStringArea(int16_t RangeX, int16_t RangeY, int16_t RangeWidth, int16_t RangeHeight,
                           int16_t X, int16_t Y, const char *String, uint8_t ChineseFontSize, uint8_t ASCIIFontSize);
void OLED_PrintfMixArea(int16_t RangeX, int16_t RangeY, int16_t RangeWidth, int16_t RangeHeight,
                       int16_t X, int16_t Y, uint8_t ChineseFontSize, uint8_t ASCIIFontSize, const char *format, ...);

// 绘图函数
void OLED_DrawPoint(int16_t X, int16_t Y);
uint8_t OLED_GetPoint(uint8_t X, uint8_t Y);
void OLED_DrawLine(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1);
void OLED_DrawRectangle(int16_t X, int16_t Y, int16_t Width, int16_t Height, uint8_t IsFilled);
void OLED_DrawTriangle(int16_t X0, int16_t Y0, int16_t X1, int16_t Y1, int16_t X2, int16_t Y2, uint8_t IsFilled);
void OLED_DrawCircle(int16_t X, int16_t Y, int16_t Radius, uint8_t IsFilled);
void OLED_DrawEllipse(int16_t X, int16_t Y, int16_t A, int16_t B, uint8_t IsFilled);
void OLED_DrawArc(int16_t X, int16_t Y, int16_t Radius, int16_t StartAngle, int16_t EndAngle, uint8_t IsFilled);
void OLED_DrawRoundedRectangle(int16_t X, int16_t Y, int16_t Width, int16_t Height, int16_t Radius, uint8_t IsFilled);

#ifdef __cplusplus
}
#endif

#endif
