/**
 * @brief 浮点数格式化对比测试
 * 
 * 用于对比 printf、sprintf、vsprintf 的浮点支持差异
 */

#include <stdio.h>
#include <stdarg.h>
#include "usart.h"  // 用于串口打印

// 模拟 OLED_PrintfMixArea 的实现
void test_vsprintf_float(const char *format, float value) {
    char buffer[128];
    va_list args;
    
    va_start(args, format);
    vsprintf(buffer, format, args);  // ← 关键测试点
    va_end(args);
    
    // 通过串口输出结果
    printf("vsprintf result: [%s]\r\n", buffer);
}

// 使用 sprintf（不经过 va_list）
void test_sprintf_float(const char *format, float value) {
    char buffer[128];
    sprintf(buffer, format, value);  // ← 对比测试
    printf("sprintf result: [%s]\r\n", buffer);
}

// 直接使用 printf
void test_printf_float(const char *format, float value) {
    printf("printf result: ");
    printf(format, value);  // ← 对比测试
    printf("\r\n");
}

void run_float_format_tests(void) {
    float test_val = 0.5f;
    const char *test_format = "T_Kp:%+4.3f";
    
    printf("\r\n========== Float Format Test ==========\r\n");
    printf("Test value: %f\r\n", test_val);
    printf("Format string: %s\r\n\r\n", test_format);
    
    // 测试 1：直接 printf
    test_printf_float(test_format, test_val);
    
    // 测试 2：sprintf
    test_sprintf_float(test_format, test_val);
    
    // 测试 3：vsprintf（模拟 OLED_PrintfMixArea）
    test_vsprintf_float(test_format, test_val);
    
    printf("\r\n========================================\r\n");
    printf("Expected: T_Kp:+0.500\r\n");
    printf("If vsprintf shows 'T_Kp:' only → NO FLOAT SUPPORT!\r\n");
    printf("========================================\r\n");
}
