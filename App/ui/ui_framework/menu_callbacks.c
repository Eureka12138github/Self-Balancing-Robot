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
#include "Delay.h"
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
 * @brief 传感器自动校准回调函数（通过 UI 菜单触发）
 * 
 * @details
 * 用户操作流程：
 * 1. 将小车保持直立静止状态（重要！）
 * 2. 通过 UI 菜单进入"系统校准" → "执行校准"
 * 3. 此函数自动采集 100 次数据并计算平均值
 * 4. 校准结果自动保存到全局变量
 * 5. LED 闪烁提示校准状态
 * 
 * @note 
 * - 采样次数 N=100
 * - 校准期间会暂时关闭电机输出，确保安全
 * - 校准完成后需重启或重新使能 run_flag 才能继续运行
 */
void Calibration_Callback(void) {
    // 安全检查：防止重入
    if (s_is_calibrating) {
        return;
    }
    
    s_is_calibrating = true;
    
    // ==================== 步骤 1：安全准备 ====================
    
    // 保存当前运行状态
    static bool s_prev_run_flag = false;
    s_prev_run_flag = run_flag;
    
    // 暂停小车运行（关闭电机）
    run_flag = false;
    Motor_SetSpeed(MOTOR_LEFT, 0);
    Motor_SetSpeed(MOTOR_RIGHT, 0);
    
    // ==================== 步骤 2：数据采集 ====================
    
    const uint8_t SAMPLE_COUNT = 100;  // 采样次数
    int32_t sum_gyro_y = 0;           // 陀螺仪 Y 轴累加和
    float sum_angle_acc = 0.0f;       // 加速度计角度累加和
    
    for (uint8_t i = 0; i < SAMPLE_COUNT; i++) {
				IWDG_ReloadCounter(); 
        int16_t gyro_y_raw;
        float angle_acc_raw;
        
        // 读取传感器数据
        Get_gy_angleAcc(&gyro_y_raw, &angle_acc_raw);
        
        // 累加
        sum_gyro_y += gyro_y_raw;
        sum_angle_acc += angle_acc_raw;
        
        // 延时 5ms，模拟控制周期，避免采样过快
        // 注意：此处不能使用中断等待，应使用简单延时
        Delay_ms(5);
    }
    
    // ==================== 步骤 3：计算平均值 ====================
    
    g_gyro_y_offset = (int16_t)(sum_gyro_y / SAMPLE_COUNT);
    g_angleAcc_offset = sum_angle_acc / SAMPLE_COUNT;
    
    // ==================== 步骤 4：反馈提示 ====================
    
    // LED 闪烁 3 次提示校准完成
    for (uint8_t k = 0; k < 3; k++) {
				IWDG_ReloadCounter(); 
        GPIOC->BRR = GPIO_Pin_13; // LED ON
        Delay_ms(100);
				GPIOC->BSRR = GPIO_Pin_13;       // LED OFF
        Delay_ms(100);
    }
    
    // ==================== 步骤 5：恢复状态 ====================
    
    // 恢复之前的运行标志（如果之前是运行的）
    if (s_prev_run_flag) {
				GPIOC->BRR = GPIO_Pin_13; 
        run_flag = true;
    }
    
    // 清除校准状态
    s_is_calibrating = false;
    
}
