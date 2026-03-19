/**
 * @file float_printf_wrapper.c
 * @brief Newlib-Nano 浮点格式化透明包装器 - 实现
 * 
 * 核心算法：
 * 1. 检测格式字符串中的 %f 说明符
 * 2. 提取浮点数参数
 * 3. 手动格式化为 "整数。小数" 形式
 * 4. 应用宽度、精度、符号等格式要求
 * 5. 替换原格式字符串中的 %f 部分
 * 
 * @author Eureka
 * @date 2026-03-19
 */

#include "float_printf_wrapper.h"
#include <string.h>
#include <stdio.h>
#include <math.h>

/*============================================================================
 *                          辅助函数
 *============================================================================*/

/**
 * @brief 计算整数的十进制位数
 */
static int count_digits(int value) {
    if (value == 0) return 1;
    
    int count = 0;
    if (value < 0) {
        value = -value;
        count++; // 负号占一位
    }
    
    while (value > 0) {
        value /= 10;
        count++;
    }
    
    return count;
}

/**
 * @brief 计算 10 的 n 次方
 */
static int power_of_10(int n) {
    int result = 1;
    for (int i = 0; i < n; i++) {
        result *= 10;
    }
    return result;
}

/*============================================================================
 *                          核心 API 实现
 *============================================================================*/

bool Format_HasFloatSpecifier(const char* format) {
    if (!format) return false;
    
    const char* p = format;
    while (*p) {
        if (*p == '%') {
            p++;
            // 跳过标志位
            while (*p == '+' || *p == '-' || *p == ' ' || *p == '#' || *p == '0') {
                p++;
            }
            // 跳过宽度
            while (*p >= '0' && *p <= '9') {
                p++;
            }
            // 跳过精度
            if (*p == '.') {
                p++;
                while (*p >= '0' && *p <= '9') {
                    p++;
                }
            }
            // 跳过长度修饰符
            if (*p == 'l' || *p == 'h' || *p == 'L') {
                p++;
            }
            // 检查是否为 'f'
            if (*p == 'f') {
                return true;
            }
        }
        p++;
    }
    
    return false;
}

const char* Extract_FloatFormat(const char* format, char* out_format, size_t max_len) {
    if (!format || !out_format || max_len == 0) {
        return NULL;
    }
    
    const char* p = format;
    while (*p) {
        if (*p == '%') {
            const char* start = p;
            p++;
            
            // 跳过标志位
            while (*p == '+' || *p == '-' || *p == ' ' || *p == '#' || *p == '0') {
                p++;
            }
            // 跳过宽度
            while (*p >= '0' && *p <= '9') {
                p++;
            }
            // 跳过精度
            if (*p == '.') {
                p++;
                while (*p >= '0' && *p <= '9') {
                    p++;
                }
            }
            // 跳过长度修饰符
            if (*p == 'l' || *p == 'h' || *p == 'L') {
                p++;
            }
            
            // 检查是否为 'f'
            if (*p == 'f') {
                // 提取格式说明符
                size_t len = p - start + 1;
                if (len >= max_len) {
                    len = max_len - 1;
                }
                strncpy(out_format, start, len);
                out_format[len] = '\0';
                return start;
            }
        }
        p++;
    }
    
    return NULL;
}

bool Parse_FloatFormat(const char* format, char* out_flags, int* out_width, int* out_precision) {
    if (!format || *format != '%') {
        return false;
    }
    
    const char* p = format + 1;
    
    // 解析标志位
    if (out_flags) {
        *out_flags = '\0';
        char* flag_ptr = out_flags;
        
        while (*p == '+' || *p == '-' || *p == ' ' || *p == '#' || *p == '0') {
            *flag_ptr++ = *p++;
        }
        *flag_ptr = '\0';
    } else {
        while (*p == '+' || *p == '-' || *p == ' ' || *p == '#' || *p == '0') {
            p++;
        }
    }
    
    // 解析宽度
    if (out_width) {
        *out_width = 0;
        while (*p >= '0' && *p <= '9') {
            *out_width = *out_width * 10 + (*p - '0');
            p++;
        }
    } else {
        while (*p >= '0' && *p <= '9') {
            p++;
        }
    }
    
    // 解析精度
    if (out_precision) {
        *out_precision = -1; // 默认无精度限制
        if (*p == '.') {
            p++;
            *out_precision = 0;
            while (*p >= '0' && *p <= '9') {
                *out_precision = *out_precision * 10 + (*p - '0');
                p++;
            }
        }
    } else {
        if (*p == '.') {
            p++;
            while (*p >= '0' && *p <= '9') {
                p++;
            }
        }
    }
    
    // 跳过长度修饰符
    if (*p == 'l' || *p == 'h' || *p == 'L') {
        p++;
    }
    
    // 必须是 'f'
    if (*p != 'f') {
        return false;
    }
    
    return true;
}

int Float_Format(float value, const char* format, char* buffer, size_t buffer_size) {
    if (!format || !buffer || buffer_size == 0) {
        return -1;
    }
    
    // 解析格式说明符
    char flags = '\0';
    int width = 0;
    int precision = -1;
    
    if (!Parse_FloatFormat(format, &flags, &width, &precision)) {
        // 格式错误，直接返回
        buffer[0] = '\0';
        return -1;
    }
    
    // 设置默认精度
    if (precision < 0) {
        precision = 6; // printf 默认精度
    }
    if (precision > MAX_DECIMAL_PLACES) {
        precision = MAX_DECIMAL_PLACES;
    }
    
    // 分离整数和小数部分
    int integer_part = (int)value;
    float abs_value = fabsf(value);
    int decimal_part = (int)((abs_value - (float)integer_part) * power_of_10(precision) + 0.5f);
    
    // 处理负数
    bool is_negative = value < 0;
    if (decimal_part < 0) {
        decimal_part = -decimal_part;
    }
    
    // 临时缓冲区
    char temp_buffer[FLOAT_BUFFER_SIZE];
    int written = 0;
    
    // 构建格式化字符串
    if (is_negative) {
        written = snprintf(temp_buffer, sizeof(temp_buffer), "-%d.", integer_part < 0 ? -integer_part : 0);
    } else if (flags == '+') {
        written = snprintf(temp_buffer, sizeof(temp_buffer), "+%d.", integer_part);
    } else if (flags == ' ') {
        written = snprintf(temp_buffer, sizeof(temp_buffer), " %d.", integer_part);
    } else {
        written = snprintf(temp_buffer, sizeof(temp_buffer), "%d.", integer_part);
    }
    
    // 添加小数部分（带前导零）
    for (int i = precision - 1; i >= 0; i--) {
        int divisor = power_of_10(i);
        int digit = (decimal_part / divisor) % 10;
        if (written < FLOAT_BUFFER_SIZE - 1) {
            temp_buffer[written++] = '0' + digit;
        }
    }
    temp_buffer[written] = '\0';
    
    // 应用宽度填充
    int current_len = strlen(temp_buffer);
    if (width > 0 && current_len < width) {
        // 需要填充
        int padding = width - current_len;
        char final_buffer[FLOAT_BUFFER_SIZE];
        
        // 判断是否左对齐
        bool left_align = false;
        if (flags != '\0') {
            // flags 是字符串，不是单个字符
            if (strchr(flags, '-') != NULL) {
                left_align = true;
            }
        }
        
        if (left_align) {
            // 左对齐：内容 + 空格
            snprintf(final_buffer, sizeof(final_buffer), "%-*s", width, temp_buffer);
        } else {
            // 右对齐：空格/0 + 内容
            if (strchr(flags, '0')) {
                // 用 '0' 填充
                memset(final_buffer, '0', padding);
                strcpy(final_buffer + padding, temp_buffer);
            } else {
                // 用空格填充
                snprintf(final_buffer, sizeof(final_buffer), "%*s", width, temp_buffer);
            }
        }
        
        strncpy(buffer, final_buffer, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return strlen(buffer);
    } else {
        // 不需要填充
        strncpy(buffer, temp_buffer, buffer_size - 1);
        buffer[buffer_size - 1] = '\0';
        return strlen(buffer);
    }
}

void Serial_Printf_Float_Wrapper(USART_TypeDef* USARTx, const char* format, ...) {
    if (!format) {
        return;
    }
    
    // 检查是否包含浮点格式说明符
    if (!Format_HasFloatSpecifier(format)) {
        // 没有 %f，直接使用 vsnprintf
        char buffer[200];
        va_list args;
        va_start(args, format);
        vsnprintf(buffer, sizeof(buffer), format, args);
        va_end(args);
        Serial_SendString(USARTx, buffer);
        return;
    }
    
    // 有 %f，需要特殊处理
    // 简单策略：逐个字符处理，遇到 %f 就格式化
    
    char output_buffer[300];
    int output_pos = 0;
    
    const char* p = format;
    va_list args;
    va_start(args, format);
    
    while (*p && output_pos < sizeof(output_buffer) - 1) {
        if (*p == '%') {
            // 可能是格式说明符
            const char* start = p;
            p++;
            
            // 跳过标志位
            while (*p == '+' || *p == '-' || *p == ' ' || *p == '#' || *p == '0') {
                p++;
            }
            // 跳过宽度
            while (*p >= '0' && *p <= '9') {
                p++;
            }
            // 跳过精度
            if (*p == '.') {
                p++;
                while (*p >= '0' && *p <= '9') {
                    p++;
                }
            }
            // 跳过长度修饰符
            if (*p == 'l' || *p == 'h' || *p == 'L') {
                p++;
            }
            
            // 检查类型
            if (*p == 'f') {
                // 浮点数格式
                float value = va_arg(args, double); // float 会被提升为 double
                
                // 提取格式说明符
                char float_format[FLOAT_FORMAT_MAX_LENGTH];
                size_t fmt_len = p - start + 1;
                if (fmt_len < FLOAT_FORMAT_MAX_LENGTH) {
                    strncpy(float_format, start, fmt_len);
                    float_format[fmt_len] = '\0';
                    
                    // 格式化浮点数
                    char float_buffer[FLOAT_BUFFER_SIZE];
                    Float_Format(value, float_format, float_buffer, sizeof(float_buffer));
                    
                    // 添加到输出
                    int len = strlen(float_buffer);
                    if (output_pos + len < sizeof(output_buffer) - 1) {
                        strcpy(output_buffer + output_pos, float_buffer);
                        output_pos += len;
                    }
                }
            } else {
                // 其他格式，使用 vsnprintf 处理单个参数
                // 这里简化处理，实际可能需要更复杂的逻辑
                char single_format[8];
                int fmt_len = p - start + 1;
                if (fmt_len < 8) {
                    strncpy(single_format, start, fmt_len);
                    single_format[fmt_len] = '\0';
                    
                    char temp_buffer[64];
                    switch (*p) {
                        case 'd':
                        case 'i':
                            snprintf(temp_buffer, sizeof(temp_buffer), single_format, va_arg(args, int));
                            break;
                        case 'u':
                            snprintf(temp_buffer, sizeof(temp_buffer), single_format, va_arg(args, unsigned int));
                            break;
                        case 'x':
                        case 'X':
                            snprintf(temp_buffer, sizeof(temp_buffer), single_format, va_arg(args, unsigned int));
                            break;
                        case 'c':
                            snprintf(temp_buffer, sizeof(temp_buffer), single_format, va_arg(args, int));
                            break;
                        case 's':
                            snprintf(temp_buffer, sizeof(temp_buffer), single_format, va_arg(args, char*));
                            break;
                        default:
                            // 未知格式，复制原样
                            strncpy(temp_buffer, start, p - start + 1);
                            temp_buffer[p - start + 1] = '\0';
                            break;
                    }
                    
                    int len = strlen(temp_buffer);
                    if (output_pos + len < sizeof(output_buffer) - 1) {
                        strcpy(output_buffer + output_pos, temp_buffer);
                        output_pos += len;
                    }
                }
            }
            p++;
        } else {
            // 普通字符
            output_buffer[output_pos++] = *p++;
        }
    }
    
    va_end(args);
    output_buffer[output_pos] = '\0';
    
    Serial_SendString(USARTx, output_buffer);
}
