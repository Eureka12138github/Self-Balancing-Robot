/**
 * @file main.c
 * @brief 系统主程序入口
 * @author Eureka
 * @date 2026-02-22
 * 
 * 初始化系统，并进入主循环。
 * 
 * OLED 接线说明：
 *   GND → GND
 *   VCC → 3.3V
 *   SCL → PB8
 *   SDA → PB9
 

 * DHT11接线：
 *	DATA → GPIO_Pin_12
 */

#include "menu.h"
#include "task_sched.h"
#include "System_Init.h"


/**
 * @brief 主函数入口
 */
int main(void)
{

    /* ===== 系统初始化 ===== */
    Initialize_System();

    /* ===== 主循环 ===== */
    while (1)
    {
//		g_rp_value1 = RP_GetValue(1);
//		g_rp_value2 = RP_GetValue(2);
//		g_rp_value3 = RP_GetValue(3);
//		g_rp_value4 = RP_GetValue(4);
        TaskHandler();                // 执行周期性任务（如传感器读取、按键扫描等）
        MyOLED_UI_MainLoop();         // 刷新 UI 并处理按键事件
        IWDG_ReloadCounter();         // 喂狗（必须在主循环中定期调用）
    }
}

/* ======================== 中断服务函数 ======================== */

/**
 * @brief TIM2 中断服务函数
 *
 * 用于驱动按键扫描与任务调度器的时间基准。
 */
void TIM2_IRQHandler(void)
{
    if (TIM_GetITStatus(TIM2, TIM_IT_Update) != RESET)
    {
        TaskSchedule();               // 更新任务调度计时器
        TIM_ClearITPendingBit(TIM2, TIM_IT_Update);
    }
}
