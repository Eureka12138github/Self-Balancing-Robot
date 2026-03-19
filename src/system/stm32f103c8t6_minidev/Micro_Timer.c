#include "Micro_Timer.h"
#include "stm32f10x.h"                  // Device header

/**
 * @file Micro_Timer.c
 * @brief 基于 STM32F103 TIM3 的微秒级高精度计时模块
 * 
 * @details
 * - 时钟源：APB1 (72MHz)
 * - 分频系数：72 (Prescaler = 71)，计数频率 1MHz (1 tick = 1us)
 * - 计数范围：0 ~ 65535 (16位自动重装载)，最大单次测量跨度约 65.5ms
 * - 溢出处理：支持硬件自然溢出，软件通过无符号减法自动校正跨零点误差
 * 
 * @warning 
 * 1. 本模块仅用于短时任务耗时测量（<65ms），不可用于长时计时。
 * 2. 请勿在 TIM3 中断中修改计数器值，以免干扰测量。
 */

// 全局变量，用于存储最后一次测量的耗时 (单位：us)
volatile uint32_t g_last_exec_time_us = 0;

// 报警标志：当检测到耗时异常 (>60ms) 或逻辑溢出时置 1
volatile uint8_t g_timer_overflow_flag = 0;

/**
 * @brief 初始化微秒计时器 (TIM3)
 * 
 * @details
 * 配置 TIM3 为向上计数模式，预分频 72，实现 1us 计数精度。
 * 初始化后立即清零计数器并启动定时器。
 * 
 * @note 需在 main() 函数最开始调用一次。
 */
void MicroTimer_Init(void) {
    // 1. 开启 TIM3 时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM3, ENABLE);
    
    TIM_TimeBaseInitTypeDef TIM_TimeBaseStructure;
    
    // 2. 配置时基结构
    // 72MHz / (72-1+1) = 1MHz -> 1us per tick
    TIM_TimeBaseStructure.TIM_Prescaler = 72 - 1;      
    
    // 自动重装载值 0xFFFF (65535)，最大计数周期 ~65.5ms
    TIM_TimeBaseStructure.TIM_Period = 0xFFFF;         
    TIM_TimeBaseStructure.TIM_CounterMode = TIM_CounterMode_Up;
    TIM_TimeBaseStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    
    // 3. 应用配置
    TIM_TimeBaseInit(TIM3, &TIM_TimeBaseStructure);
    
    // 4. 【关键】强制清零计数器，确保从 0 开始计数
    TIM_SetCounter(TIM3, 0); 
    
    // 5. 启动定时器
    TIM_Cmd(TIM3, ENABLE);
}

/**
 * @brief 获取当前微秒时间戳
 * 
 * @return uint32_t 当前 TIM3 计数值 (0 ~ 65535)，单位 us。
 * 
 * @note 
 * - 返回值虽为 uint32_t，但有效数据仅在低 16 位。
 * - 该函数执行极快（直接读寄存器），可安全用于高频调用。
 */
uint32_t Get_Micros(void) {
    return TIM3->CNT;
}

/**
 * @brief 安全计算两点之间的时间差 (处理溢出)
 * 
 * @param start 起始时间戳 (由 Get_Micros() 获取)
 * @param end   结束时间戳 (由 Get_Micros() 获取)
 * 
 * @return uint32_t 耗时微秒数。
 *         - 正常情况：返回实际差值 (0 ~ 60000)。
 *         - 异常情况：若差值 > 60000us (认为溢出多次或逻辑错误)，返回 65535 并置位全局报警标志。
 * 
 * @details
 * **溢出处理原理**：
 * 利用无符号整数减法的模运算特性。即使计数器从 65535 回绕到 0，
 * 只要 (end - start) 的真实物理时间差小于 65.5ms，直接相减即可得到正确结果。
 * 
 * 示例：
 * start = 65530, end = 10 (已溢出)
 * 计算：(uint16_t)(10 - 65530) = (uint16_t)(-65520) = 10 (正确!)
 * 
 * @warning 必须确保 start 和 end 是在一次完整的测量周期内获取的。
 */
uint32_t Calculate_Elapsed_Time(uint32_t start, uint32_t end) {
    // 强制转换为 16 位进行减法，利用硬件溢出特性自动校正跨零点误差
    // 然后提升回 32 位以便后续比较
    uint32_t delta = (uint16_t)(end - start); 
    
    // 安全检查阈值：60ms (留 5ms 余量防止边界问题)
    if (delta > 60000) {
        g_timer_overflow_flag = 1; // 标记异常：可能是任务卡死或测量逻辑错误
        return 65535;              // 返回最大值作为错误指示
    }
    
    g_timer_overflow_flag = 0;     // 清除标志
    return delta;
}

/* ==========================================
   【使用指南 / Usage Example】
   ==========================================
   
   1. 初始化 (在 main 函数开头):
      --------------------------------------
      int main(void) {
          SystemInit();
          MicroTimer_Init();  // <--- 必须先初始化
          
          while(1) {
              // ... 主循环代码 ...
          }
      }

   2. 测量单个函数耗时:
      --------------------------------------
      void Test_Function_Speed(void) {
          uint32_t start, end, duration;
          
          start = Get_Micros();           // <--- 记录开始时间
          
          // --- 待测代码块 ---
          My_Complex_Algorithm();
          // ------------------
          
          end = Get_Micros();             // <--- 记录结束时间
          duration = Calculate_Elapsed_Time(start, end);
          
          if (g_timer_overflow_flag) {
              // 处理异常：任务执行超时！
              LED_Error_Blink();
          } else {
              // 正常：duration 即为耗时 (us)
              // 例如：duration = 380 表示耗时 380us
          }
      }

   3. 测量周期性任务负载率 (如在中断中):
      --------------------------------------
      void TIM1_UP_IRQHandler(void) {
          static uint32_t last_start_time = 0;
          uint32_t current_time, exec_time;
          
          current_time = Get_Micros();
          
          // 计算上一次任务的执行时间 (如果记录了 start_time)
          // 注意：通常我们在任务开始时记录 start，结束时计算
          
          // --- 任务开始 ---
          uint32_t task_start = Get_Micros();
          
          Balance_Control_Loop(); // 执行控制算法
          
          // --- 任务结束 ---
          uint32_t task_end = Get_Micros();
          exec_time = Calculate_Elapsed_Time(task_start, task_end);
          
          g_last_exec_time_us = exec_time; // 存入全局变量供调试查看
          
          // 清除中断标志...
          TIM_ClearITPendingBit(TIM1, TIM_IT_Update);
      }
      
   4. 调试技巧:
      - 将 g_last_exec_time_us 添加到调试_watch 窗口，实时观察耗时波动。
      - 如果 g_timer_overflow_flag 变 1，说明某处代码卡死或耗时超过 60ms，需立即排查。
*/
