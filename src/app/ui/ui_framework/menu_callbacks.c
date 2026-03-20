/**
 * @file menu_callbacks.c
 * @brief 菜单系统回调函数实现文件
 * 
 * 本文件集中管理所有菜单项的回调函数实现。
 * 回调函数用于响应用户的确认操作(Enter键)，执行相应的业务逻辑。
 * 
 * 使用场景说明：
 * - 当菜单项需要执行特定功能时，应在此文件中实现对应的回调函数
 * - 在my_menu_data.c中将函数指针关联到相应的菜单项
 * - 支持硬件控制、数据处理、状态切换等各种业务逻辑
 * 
 * @author Eureka & Lingma
 * @date 2026-02-22
 */

#include "menu_callbacks.h"
#include "LED.h"
#include "system_config.h"
#include "Motor.h"
#include "MPU6050.h"
#include "delay.h"  // 使用定时器实现的精确延时
#include "usart.h"  // 用于串口调试输出
// #include "delay_verify.h"  // Delay_ms() 精度验证（测试已完成，已删除）
/*============================================================================
 *                          测试回调函数实现
 *============================================================================*/

/**
 * @brief 测试回调函数1
 * 
 * 当用户在菜单中选择并确认此项时执行
 * 主要功能：切换LED状态(开/关)
 * 
 * 触发时机：用户导航到对应菜单项并按下Enter键
 * 执行动作：调用Led_Turn()函数翻转LED状态
 */
void Test_Callback_1(void) {
		run_flag = !run_flag;
		if(run_flag) {
			LED1_ON();
		}else {
			LED1_OFF();
		}
}

/** 
 * @brief 校准状态标志
 * @note true=正在校准，false=校准完成/未校准
 */
static volatile bool s_is_calibrating = false;

/**
 * @brief 传感器校准回调函数（使用互补滤波后的角度）
 * 
 * 功能：采集静止状态下的陀螺仪零偏和加速度计角度零偏
 * 校准流程：
 * 1. 暂停小车运行
 * 2. 连续采样 100 次互补滤波后的数据（5ms/次）
 * 3. 计算平均值作为零偏
 * 4. LED 闪烁 3 次提示完成
 * 
 * @note 必须将小车放置在水平面上进行校准
 * @warning 校准过程中严禁移动或触碰小车
 */
void Calibration_Callback(void) {
    // 安全检查：防止重入
    if (s_is_calibrating) {
        return;
    }
    
    s_is_calibrating = true;
 
   // 添加调试输出
    // Serial_Printf(USART_DEBUG, "\r\n=== 开始校准 ===\r\n");
    
    // ==================== 步骤 1：安全准备 ====================
    
    // 保存当前运行状态
    static bool s_prev_run_flag = false;
    s_prev_run_flag = run_flag;
    
    // 暂停小车运行（关闭电机）
    run_flag = false;
    Motor_SetSpeed(MOTOR_LEFT, 0);
    Motor_SetSpeed(MOTOR_RIGHT, 0);
    
    // Serial_Printf(USART_DEBUG, "已关闭电机\r\n");
    
    // ==================== 步骤 2：数据采集 ====================
    
    const uint8_t SAMPLE_COUNT = 100;  // 采样次数
    int32_t sum_gyro_y = 0;           // 陀螺仪 Y 轴累加和
    float sum_angle = 0.0f;           // 互补滤波后角度累加和
    
    // Serial_Printf(USART_DEBUG, "开始采样 %d 次...\r\n", SAMPLE_COUNT);
    
    // 声明外部变量（在 control.c 中定义）
    extern int16_t GY;           // 校正后的陀螺仪 Y 值
    extern float angle;          // 互补滤波后的融合角度
    
    for (uint8_t i = 0; i < SAMPLE_COUNT; i++) {
        IWDG_ReloadCounter(); 
        
        // ✅ 关键改进：使用控制循环中已经处理好的数据
        // 这些数据已经减去了之前的零偏，更准确
        sum_gyro_y += GY;
        sum_angle += angle;  // 使用互补滤波后的角度
        
        // 每 10 次输出一次进度（调试用，已禁用）
        // if (i % 10 == 0) {
        //     Serial_Printf(USART_DEBUG, "进度：%d/100, GY=%d, angle=%.2f\r\n", 
        //                  i, GY, angle);
        // }
        
        // 延时 5ms，与控制周期同步
        Delay_ms(5);
    }
    
    // Serial_Printf(USART_DEBUG, "采样完成！\r\n");
    
    // ==================== 步骤 3：计算平均值 ====================
    
    // 计算陀螺仪零偏
    // 原理：校准时小车静止，GY 应该为 0
    // 所以零偏 = 采样平均值（这样 gy -= offset 后才为 0）
    g_gyro_y_offset = (int16_t)(sum_gyro_y / SAMPLE_COUNT);
    
    // 计算角度零偏
    // 原理：校准时小车直立，理论角度应该为 0
    // 所以零偏 = 采样平均值（这样 angleAcc -= offset 后才为 0）
    // ⚠️ 注意：这里不需要负号！
    g_angleAcc_offset = sum_angle / SAMPLE_COUNT;
    
    // Serial_Printf(USART_DEBUG, "\r\n=== 校准结果 ===\r\n");
    // Serial_Printf(USART_DEBUG, "GY 零偏：%d\r\n", g_gyro_y_offset);
    // Serial_Printf(USART_DEBUG, "Angle 零偏：%.3f\r\n", g_angleAcc_offset);
    // Serial_Printf(USART_DEBUG, "原始总和：GY=%ld, Angle=%.2f\r\n", 
    //              (long)sum_gyro_y, sum_angle);
    
    // ==================== 步骤 4：反馈提示 ====================
    
    // Serial_Printf(USART_DEBUG, "LED 开始闪烁...\r\n");
    
    // LED 闪烁 3 次提示校准完成
    // 采用"亮短灭长"模式，更容易肉眼分辨
    for (uint8_t k = 0; k < 3; k++) {
        IWDG_ReloadCounter(); 
        GPIOC->BRR = GPIO_Pin_13;     // LED ON
        Delay_ms(100);  // 亮 100ms
        
        GPIOC->BSRR = GPIO_Pin_13;    // LED OFF
        Delay_ms(100);  // 灭 100ms
    }
    
    // Serial_Printf(USART_DEBUG, "=== 校准完成 ===\r\n");
    
    // ==================== 步骤 5：恢复状态 ====================
    
    // 恢复之前的运行标志（如果之前是运行的）
    if (s_prev_run_flag) {
        run_flag = true;
    }
    
    // 清除校准状态
    s_is_calibrating = false;
    
}
