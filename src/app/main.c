/**
 * @file main.c
 * @brief 系统主程序入口
 * @author Eureka
 * @date 2026-03-12
 * 
 * 层次依赖说明:
 * - 直接依赖：app/ 下的应用模块（menu, task_sched）
 * - 间接依赖：drivers/ 和 system/ 通过 System_Init.h 初始化
 */

#include "menu.h"           // UI 菜单
#include "task_sched.h"     // 任务调度
#include "System_Init.h"    // 系统初始化（封装底层驱动）
#include "control.h"        // 控制算法


void SystemClock_Config(void);



/**
 * @brief  主函数
 */
int main(void)
{
    /* 系统时钟初始化 - 框架的 SystemInit 只配置向量表 */
    SystemInit();
    
    /* 手动配置系统时钟为 72MHz */
    SystemClock_Config();

    /* ===== 系统初始化 ===== */
    Initialize_System();
    
    /* 平衡控制模块初始化 */
    Balance_Init();

    /* 主循环 */
    while(1)
    {

            
		TaskHandler();                // 任务调度（按键扫描、状态机等）
		MyOLED_UI_MainLoop();         // UI 刷新
              
        // IWDG_ReloadCounter();         // 喂狗（防止看门狗复位）        
    }
}


/* ======================== 中断服务函数 ======================== */

void TIM1_UP_IRQHandler(void)//每 5ms 触发一次
{
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)
    {
        TaskSchedule(); 
        
        // ============ 调用平衡控制调度器 ============
        Balance_Control_Scheduler();
        
        TIM_ClearITPendingBit(TIM1, TIM_IT_Update) ;
    }
}

/**
 * @brief  系统时钟配置函数 - 72MHz 最高性能配置
 *         
 * 【时钟树配置】
 *   - HSE: 8MHz (外部晶振)
 *   - PLL: HSE × 9 = 72MHz
 *   - SYSCLK: 72MHz (系统时钟，芯片上限)
 *   - HCLK:   72MHz (AHB 总线，芯片上限)
 *   - PCLK2:  72MHz (APB2 高速外设总线，上限 72MHz)
 *             → TIM1, SPI1, USART1, GPIO 等
 *   - PCLK1:  36MHz (APB1 低速外设总线，上限 36MHz)
 *             → TIM2-7, I2C1-2, SPI2-3, USART2-3, CAN, USB 等
 *   - ADCCLK: 12MHz (ADC 时钟，最高支持 14MHz)
 *   
 * 【Flash 配置】
 *   - Latency: 2WS (72MHz 必需)
 *   - Prefetch Buffer: Enable (提升性能)
 *   
 * 【适用场景】
 *   ✓ 通用高性能应用
 *   ✓ 需要最大计算能力
 *   ✓ 所有外设运行在允许的最高频率
 *   
 * 【与 Keil 标准库对比】
 *   ✓ 完全等效于 Keil MDK 中 STM32F10x 标准库的默认配置
 *   ✓ 达到"一次配置、长期使用"的效果
 *   
 * @param  None
 * @retval None
 */
void SystemClock_Config(void)
{
    /* 复位 RCC 到默认状态 */
    RCC_DeInit();
    
    /* 使能外部高速时钟 HSE */
    RCC_HSEConfig(RCC_HSE_ON);
    
    /* 等待 HSE 就绪 */
    if (RCC_WaitForHSEStartUp() == SUCCESS)
    {
        /* 配置 Flash 等待周期：72MHz 需要 2 个等待周期 */
        FLASH_SetLatency(FLASH_Latency_2);
        
        /* 使能 Flash 预取缓冲区以提高性能 */
        FLASH_PrefetchBufferCmd(FLASH_PrefetchBuffer_Enable);
        
        /* 配置 HCLK = SYSCLK = 72MHz */
        RCC_HCLKConfig(RCC_SYSCLK_Div1);
        
        /* 配置 PCLK2 = HCLK = 72MHz */
        RCC_PCLK2Config(RCC_HCLK_Div1);
        
        /* 配置 PCLK1 = HCLK/2 = 36MHz */
        RCC_PCLK1Config(RCC_HCLK_Div2);
        
        /* 配置 ADC 时钟 = PCLK2/6 = 12MHz (最高支持 14MHz，12MHz 更保守安全)
         * 如需更高 ADC 采样率，可改为 RCC_PCLK2_Div4 = 14.4MHz */
        RCC_ADCCLKConfig(RCC_PCLK2_Div6);
        
        /* 配置 PLL 时钟源和倍频系数：PLLCLK = HSE × 9 = 72MHz */
        RCC_PLLConfig(RCC_PLLSource_HSE_Div1, RCC_PLLMul_9);
        
        /* 使能 PLL */
        RCC_PLLCmd(ENABLE);
        
        /* 等待 PLL 就绪 */
        while (RCC_GetFlagStatus(RCC_FLAG_PLLRDY) == RESET)
        {
        }
        
        /* 选择 PLL 作为系统时钟源 */
        RCC_SYSCLKConfig(RCC_SYSCLKSource_PLLCLK);
        
        /* 等待 PLL 被选为系统时钟 */
        while (RCC_GetSYSCLKSource() != 0x08)
        {
        }
        
        /* 【关键】更新 SystemCoreClock 变量为 72MHz */
        SystemCoreClock = 72000000;  // 72MHz
        
    }
}