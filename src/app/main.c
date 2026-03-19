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
#include "system_config.h"  // 系统全局配置（包含 PID、速度等全局变量声明）
#include "control.h"        // 控制算法（包含 Balance_Control_Loop 等）
#include "hall_encoder.h"   // 霍尔编码器接口
#include "BlueSerial.h"     // 蓝牙串口通信
#include <string.h>         // 字符串处理
#include <stdlib.h>         // 标准库

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

    
    /* 主循环 */
    while(1)
    {
		/* 蓝牙串口接收数据处理 */
		/* 数据包格式：[joystick,LH,LV,RH,RV] */
		if (BlueSerial_RxFlag == 1)		// 如果收到数据包
		{
			char *Tag = strtok(BlueSerial_RxPacket, ",");	// 提取数据标签
			if (strcmp(Tag, "joystick") == 0)			// 摇杆数据包
			{
				int8_t LH = atoi(strtok(NULL, ","));		// 左手柄横向
				int8_t LV = atoi(strtok(NULL, ","));		// 左手柄纵向 → 速度控制
				int8_t RH = atoi(strtok(NULL, ","));		// 右手柄横向 → 转向控制
				int8_t RV = atoi(strtok(NULL, ","));		// 右手柄纵向
				
				/* 执行摇杆操作 */
				speedPID.target = LV;	// 前后行进控制
				turnPID.target = RH;		// 左右转弯控制
			}
			
			BlueSerial_RxFlag = 0;		// 清除标志，准备接收下一个包
		}
		
		TaskHandler();                // 任务调度（按键扫描、状态机等）
		MyOLED_UI_MainLoop();         // UI 刷新
        IWDG_ReloadCounter();         // 喂狗（防止看门狗复位）        
    }
}


/* ======================== 中断服务函数 ======================== */

void TIM1_UP_IRQHandler(void)//每5ms触发一次
{		static uint16_t count1;
    if (TIM_GetITStatus(TIM1, TIM_IT_Update) != RESET)
    {
			TaskSchedule(); 
			Balance_Control_Loop();//这是直立环控制
			count1++;
			if(count1 >= 8) {//每隔5 * 8ms执行一次速度环控制
				count1 = 0;
				//44是电机极数，0.04是读取周期，9.27666是减速比，这个极数和减速比不同规格的电机应不一样
				//这里是直接使用江协电机的数据，我的电机可能不是这个，将就用着先，这其实只是个比例转换
				//即使不进行速度转换，应该也没问题。现在经过转换后单位为，转/秒
				left_speed = Hall_Encoder_Get(ENCODER_LEFT) / 16.326816;// 44.0 * 0.04 * 9.27666
				right_speed = Hall_Encoder_Get(ENCODER_RIGHT) / 16.326816;
				ave_speed = (left_speed + right_speed ) / 2.0;
				dif_speed = left_speed - right_speed;
				if(run_flag) {
					//速度环
					speedPID.actual = ave_speed;
					PID_update(&speedPID);
					anglePID.target = speedPID.out;
					
					//转向环
					turnPID.actual = dif_speed;
					PID_update(&turnPID);
					dif_PWM = turnPID.out;
				}		
			}
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
    }
}