/**
 * @file float_printf_test.c
 * @brief 浮点格式化包装器测试 - 验证 Serial_Printf 的 %f 支持
 * 
 * @author Eureka
 * @date 2026-03-19
 */

#include <stdio.h>
#include "usart.h"
#include "float_printf_wrapper.h"

// 测试用的全局变量，方便调试查看
static float g_test_values[] = {3.14159f, -2.5f, 0.001f, 12345.6789f, 0.0f};
static const int NUM_TESTS = sizeof(g_test_values) / sizeof(g_test_values[0]);

/**
 * @brief 测试 1：基本 %f 格式
 */
void test_basic_float(void) {
    printf("\r\n[TEST 1] Basic %%f format:\r\n");
    
    for (int i = 0; i < NUM_TESTS; i++) {
        printf("  Value %d: %f\r\n", i, g_test_values[i]);
    }
}

/**
 * @brief 测试 2：精度控制（%.2f, %.3f）
 */
void test_precision_control(void) {
    printf("\r\n[TEST 2] Precision control:\r\n");
    
    float pi = 3.1415926f;
    
    printf("  Pi default:   %f\r\n", pi);      // 默认 6 位
    printf("  Pi %.0f:       %.0f\r\n", pi);   // 0 位小数
    printf("  Pi %.1f:       %.1f\r\n", pi);   // 1 位小数
    printf("  Pi %.2f:       %.2f\r\n", pi);   // 2 位小数
    printf("  Pi %.3f:       %.3f\r\n", pi);   // 3 位小数
    printf("  Pi %.4f:       %.4f\r\n", pi);   // 4 位小数
}

/**
 * @brief 测试 3：符号标志（%+f, % f）
 */
void test_sign_flags(void) {
    printf("\r\n[TEST 3] Sign flags:\r\n");
    
    float positive = 3.14f;
    float negative = -2.5f;
    
    printf("  Positive +%%f: %+f\r\n", positive);
    printf("  Positive %%f:   %f\r\n", positive);
    printf("  Negative +%%f: %+f\r\n", negative);
    printf("  Negative %%f:   %f\r\n", negative);
    printf("  Space flag %% f: % f\r\n", positive);
    printf("  Space flag %% f: % f\r\n", negative);
}

/**
 * @brief 测试 4：宽度控制（%10f, %-10f）
 */
void test_width_control(void) {
    printf("\r\n[TEST 4] Width control:\r\n");
    
    float val = 3.14f;
    
    printf("  Default:     [%f]\r\n", val);
    printf("  Width 10:    [%10f]\r\n", val);
    printf("  Width 15:    [%15f]\r\n", val);
    printf("  Left align:  [%-10f]\r\n", val);
    printf("  Left align:  [%-15f]\r\n", val);
}

/**
 * @brief 测试 5：零填充（%010f）
 */
void test_zero_padding(void) {
    printf("\r\n[TEST 5] Zero padding:\r\n");
    
    float small = 3.14f;
    float large = 123.456f;
    
    printf("  Small default:  [%f]\r\n", small);
    printf("  Small 0-padded: [%010f]\r\n", small);
    printf("  Large 0-padded: [%010f]\r\n", large);
    printf("  Small 015.3f:   [%015.3f]\r\n", small);
}

/**
 * @brief 测试 6：复合格式（整数 + 浮点）
 */
void test_mixed_format(void) {
    printf("\r\n[TEST 6] Mixed integer and float:\r\n");
    
    int count = 42;
    float voltage = 3.7f;
    float current = 0.25f;
    float power = voltage * current;
    
    printf("  Count: %d\r\n", count);
    printf("  Voltage: %.2f V\r\n", voltage);
    printf("  Current: %.3f A\r\n", current);
    printf("  Power: %.4f W\r\n", power);
    printf("  Combined: %d devices, %.1fV, %.2fA\r\n", count, voltage, current);
}

/**
 * @brief 测试 7：PID 参数显示（实际应用场景）
 */
void test_pid_display(void) {
    printf("\r\n[TEST 7] PID parameter display (real use case):\r\n");
    
    float Kp = 2.5f;
    float Ki = 0.1f;
    float Kd = 0.05f;
    
    // 模拟菜单显示
    printf("  [PID Settings]\r\n");
    printf("  Kp: %+6.3f\r\n", Kp);
    printf("  Ki: %+6.3f\r\n", Ki);
    printf("  Kd: %+6.3f\r\n", Kd);
    
    // 不同精度
    printf("\r\n  High precision:\r\n");
    printf("  Kp: %.6f\r\n", Kp);
    printf("  Ki: %.6f\r\n", Ki);
    printf("  Kd: %.6f\r\n", Kd);
}

/**
 * @brief 测试 8：边界值测试
 */
void test_edge_cases(void) {
    printf("\r\n[TEST 8] Edge cases:\r\n");
    
    printf("  Zero:           %f\r\n", 0.0f);
    printf("  Very small:     %.10f\r\n", 0.0000001f);
    printf("  Very large:     %f\r\n", 999999.999f);
    printf("  Negative zero:  %f\r\n", -0.0f);
    printf("  Integer value:  %f\r\n", 42.0f);
    printf("  Half:           %f\r\n", 0.5f);
}

/**
 * @brief 主测试函数
 */
void float_printf_test_main(void) {
    printf("\r\n");
    printf("========================================\r\n");
    printf("  Float Printf Wrapper Test Suite\r\n");
    printf("  Newlib-Nano Transparent Solution\r\n");
    printf("========================================\r\n");
    printf("\r\n");
    
    test_basic_float();
    test_precision_control();
    test_sign_flags();
    test_width_control();
    test_zero_padding();
    test_mixed_format();
    test_pid_display();
    test_edge_cases();
    
    printf("\r\n========================================\r\n");
    printf("  All Tests Complete!\r\n");
    printf("========================================\r\n");
    printf("\r\n");
    
    printf("Analysis Guide:\r\n");
    printf("1. Check if all %%f formats show correct values\r\n");
    printf("2. Verify precision matches specification\r\n");
    printf("3. Confirm sign flags (+, space) work correctly\r\n");
    printf("4. Validate width and padding behavior\r\n");
    printf("5. Ensure mixed formats (int + float) work together\r\n");
    printf("\r\n");
    printf("If any test shows missing numbers or wrong values,\r\n");
    printf("the wrapper may need debugging.\r\n");
    printf("\r\n");
}

/*
 * 使用方法：
 * 
 * 1. 在 main.c 中调用：
 *    #include "float_printf_test.c"
 *    
 *    int main(void) {
 *        System_Init();
 *        float_printf_test_main();
 *        while(1) { ... }
 *    }
 * 
 * 2. 编译并烧录
 * 
 * 3. 打开串口助手观察输出
 * 
 * 预期结果：
 * - 所有 %f 格式都应该正确显示数值
 * - 精度、宽度、符号等格式说明符应该正常工作
 * - 混合整数和浮点的格式应该正确解析
 * 
 * 如果看到输出中没有数值（如 "Value: "），则说明包装器未生效
 */
