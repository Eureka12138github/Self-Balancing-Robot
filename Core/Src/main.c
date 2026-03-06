/**
 * @file main.c
 * @brief 系统主程序入口
 * @author Eureka
 * @date 2026-03-02
 * 
 * 初始化系统，并进入主循环。
 * 
 * OLED 接线说明：
 *   GND → GND
 *   VCC → 3.3V
 *   SCL → PB8
 *   SDA → PB9
 
 * 电机驱动 接线说明：
 *   PWMA → PA0
 *   PB13 → AIN1
 *   PB12 → AIN2
 *   STBY → 3.3V
 
 *   PWMB → PA1
 *   PB15 → BIN1
 *   PB14 → BIN2

 *   E1A → PA7
 *   E1B → PA6
 *   E2A → PB6  
 *   E2B → PB7
 
 
 * 霍尔编码器输出：
 *	E1A → PA6
 * 	E1B → PA7
 
 * MPU6050：
 *	SDA → PB11
 * 	SCL → PB10 
 
 * 旋转编码器：
 * 	A → PB0
 * 	B → PB1 
 
 * 按键接线：
 * 	按键1 → PB12
 * 	按键2 → PB15
 * 
 * 预期现象：
 * 旋转编码器控制电机转速，串口接收到 一个范围在[-50,50]之内的值（此值可通过旋转编码器调节）
 */

#include "menu.h"
#include "task_sched.h"
#include "System_Init.h"
#include <math.h>

/**
 * @brief 主函数入口
 */
int16_t AX,AY,AZ,GX,GY,GZ;
int16_t timer_count;
float angleAcc;
float angleGyro;
float angle;
int main(void)
{
    /* ===== 系统初始化 ===== */
    Initialize_System();

    /* ===== 定义上次执行时间（单位：ms）===== */
    static uint32_t last_print_time = 0;  // 初始为 0，确保首次立即执行
    /* ===== 主循环 ===== */
    while (1)
    {
        uint32_t current_time = SysTick_Get();  // 获取当前系统时间（ms）
				Motor_SetSpeed(MOTOR_LEFT,g_rp_value1);
				Motor_SetSpeed(MOTOR_RIGHT,g_rp_value2);
        // 检查是否到达 500ms 周期
        if (current_time - last_print_time >= 500)
        {
            // 更新时间戳（关键！）
            last_print_time = current_time;

//            Serial_Printf(USART_DEBUG, "%d,%d,%d,%d,%d,%d\r\n", AX, AY, AZ, GX, GY, GZ);
        }
//				  Serial_Printf(USART_DEBUG, "%d,%d,%d,%d,%d,%d\r\n", AX, AY, AZ, GX, GY, GZ);
				Serial_Printf(USART_DEBUG, "%f,%f\r\n", angleAcc,angle);

				GY -= 48;
				TaskHandler();                // 如按键扫描、状态机等
        MyOLED_UI_MainLoop();         // UI 刷新（通常也需要非阻塞设计）
        IWDG_ReloadCounter();         // 喂狗（必须高频调用！）


    }
}

/* ======================== 中断服务函数 ======================== */

void TIM1_UP_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)
    {
			MPU6050_GetData(&AX, &AY, &AZ, &GX, &GY, &GZ);
      angleAcc = -atan2(AX,AZ) / 3.14159 * 180;  
			angleGyro = angle + GY / 32768.0 * 2000*0.001;//满量程下正负2000°/s，这里是在进行缩放，需要看MPU6050手册才可理解
			float alpha = 0.001;
			angle = alpha * angleAcc + (1 - alpha) * angleGyro;//这里是互补滤波，基于acc抑制gyro的零漂
			TaskSchedule(); 
		TIM_ClearITPendingBit(TIM1, TIM_IT_Update) ;
    }
}
