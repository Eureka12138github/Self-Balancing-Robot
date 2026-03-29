/**
 * @file System_Init.c
 * @brief 系统初始化模块 - 集中管理所有硬件初始化
 * 
 * 设计原则:
 * - 统一管理所有硬件初始化
 * - main.c 只需调用 Initialize_System()
 * - 避免 main.c 直接依赖底层驱动
 */

#include "System_Init.h"
#include "system_config.h"     // 系统全局配置
#include "alarm.h"             // 警报模块
#include "iwdg.h"              // 看门狗
#include "menu_data.h"         // 菜单数据
#include "menu.h"              // UI 菜单
#include "led.h"               // LED 驱动
#include "hall_encoder.h"      // 霍尔编码器
#include "Motor.h"             // 电机驱动
#include "usart.h"             // 串口驱动
#include "MPU6050.h"           // MPU6050 驱动
#include "Delay.h"             // 延时函数
#include "BlueSerial.h"        // 蓝牙串口
#include "PID.h"               // PID 高级功能
#include "storage.h"           // Flash 存储管理（新增）

/**
 * @brief  初始化系统。
 * @retval None
 */
void Initialize_System(void) {
    // ========== 第一阶段：Flash 存储初始化 ==========
    // 必须在所有硬件初始化之前调用，确保 PID 参数加载到内存
    Store_Init();
    
    // ========== 第二阶段：加载持久化的 PID 参数 ==========
    // 从 Flash 读取上次保存的 PID 参数（如果数据有效）
    // 这样设备重启后能自动应用上次的参数，而不是重置为默认值
    {
        // 加载直立环 PID 参数
        if (!PID_Store_Load(&anglePID, PID_ANGLE_KP_IDX)) {
            // 如果加载失败（数据无效），保持 system_config.c 中的默认值
            // 可选：添加调试输出或 LED 提示
        }
        
        // 加载速度环 PID 参数
        if (!PID_Store_Load(&speedPID, PID_SPEED_KP_IDX)) {
            // 加载失败，保持默认值
        }
        
        // 加载转向环 PID 参数
        if (!PID_Store_Load(&turnPID, PID_TURN_KP_IDX)) {
            // 加载失败，保持默认值
        }
    }
    
    // ========== 第三阶段：硬件和 UI 初始化 ==========
    MyOLED_UI_Init(&MainPage);        				// MainPage 在 my_menu_data.h 中定义	
    Alarm_Init();									//警报初始化
	Usart1_Init(9600);  // 初始化调试串口
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_4);  //优先级分组配置   
  // ⚠️ 开发阶段禁用看门狗，避免不必要的复位干扰
	// MYIWD_Init(2000);
	Motor_Init();
	Hall_Encoder_Init();
	MPU6050_Init();
	BlueSerial_Init();
	
	LED_Init();
	LED1_OFF();
	
	// 初始化 PID 高级优化功能（不完全微分 + 变速积分）
	// 注意：这里只是配置高级功能参数，基础 PID 参数已在 system_config.c 中初始化
	// 如果需要动态调整，可以使用 PID_SetAngleDiffFilterCoef 等函数
}
