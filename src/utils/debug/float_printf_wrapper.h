/**
 * @file float_printf_wrapper.h
 * @brief Newlib-Nano 浮点格式化透明包装器
 * 
 * 功能：在 Newlib-Nano 环境下，让 Serial_Printf 和 OLED_Printf 等函数
 *      支持 %f 浮点格式化，而无需修改现有调用代码
 * 
 * @author Eureka
 * @date 2026-03-19
 */

#ifndef __FLOAT_PRINTF_WRAPPER_H
#define __FLOAT_PRINTF_WRAPPER_H

#include "stm32f10x.h"  // Device header - 用于 USART_TypeDef

#ifdef __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include <stdarg.h>
#include <stdbool.h>

/*============================================================================
 *                          浮点格式化配置
 *============================================================================*/

/**
 * @brief 浮点数格式说明符最大长度
 * 
 * 例如："%+10.5f" 长度为 8
 * 预留足够空间以支持复杂格式
 */
#define FLOAT_FORMAT_MAX_LENGTH     (16)

/**
 * @brief 浮点格式化后的最大字符串长度
 * 
 * 考虑最坏情况："-999999.999999" = 15 字符 + 结束符
 */
#define FLOAT_BUFFER_SIZE           (32)

/**
 * @brief 支持的最大小数位数
 * 
 * Newlib-Nano 下手动格式化的精度限制
 * 建议不超过 6 位（float 精度极限）
 */
#define MAX_DECIMAL_PLACES          (6)

/*============================================================================
 *                          核心 API
 *============================================================================*/

/**
 * @brief 将浮点数格式化为字符串（手动实现，绕过 Newlib-Nano 限制）
 * 
 * @param value 要格式化的浮点数值
 * @param format 格式说明符（如 "%+4.3f"）
 * @param buffer 输出缓冲区
 * @param buffer_size 缓冲区大小
 * @return int 实际写入的字符数（不包括'\0'），失败返回 -1
 * 
 * @note 支持的格式：
 *       - %f, %.2f, %+4.3f, %10.5f 等标准格式
 *       - 不支持：%e, %E, %g, %G（科学计数法）
 */
int Float_Format(float value, const char* format, char* buffer, size_t buffer_size);

/**
 * @brief 检测格式化字符串是否包含浮点格式说明符
 * 
 * @param format 格式化字符串
 * @return true 包含 %f（需要特殊处理）
 * @return false 不包含 %f（可直接使用 vsnprintf）
 * 
 * @note 检测的格式：%f, %.2f, %+4.3f, %10.5f 等
 */
bool Format_HasFloatSpecifier(const char* format);

/**
 * @brief 从格式字符串中提取单个浮点格式说明符
 * 
 * @param format 完整的格式化字符串
 * @param out_format 输出提取到的浮点格式（如 "%+4.3f"）
 * @param max_len out_format 缓冲区的最大长度
 * @return const char* 指向 format 中格式说明符的起始位置
 * 
 * @note 如果找到多个 %f，只提取第一个
 */
const char* Extract_FloatFormat(const char* format, char* out_format, size_t max_len);

/**
 * @brief 解析浮点格式说明符的各个字段
 * 
 * @param format 格式说明符（如 "%+4.3f"）
 * @param out_flags 输出标志位（如 '+'、'-'、' ' 等）
 * @param out_width 输出宽度（如 4）
 * @param out_precision 输出精度（如 3）
 * @return true 解析成功
 * @return false 格式错误
 */
bool Parse_FloatFormat(const char* format, char* out_flags, int* out_width, int* out_precision);

/*============================================================================
 *                          透明包装宏（可选）
 *============================================================================*/

/**
 * @brief 重定义 Serial_Printf 为包装版本（可选）
 * 
 * 使用方法：
 * 1. 在 usart.h 中添加：#include "float_printf_wrapper.h"
 * 2. 取消下面的注释
 * 3. 所有 Serial_Printf 调用自动支持浮点数
 * 
 * @warning 这会改变原有函数的行为，需谨慎使用
 */
// #define Serial_Printf(USARTx, format, ...) \
//     Serial_Printf_Float_Wrapper(USARTx, format, ##__VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* __FLOAT_PRINTF_WRAPPER_H */
