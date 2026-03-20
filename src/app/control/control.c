#include "control.h"
#include "stm32f10x.h"                  // Device header
#include "system_config.h"
#include "Motor.h"
#include "MPU6050.h"
#include "LED.h"
#include "serial_cmd.h"  // 串口命令解析
#include "usart.h"       // 串口驱动（包含 USART_DEBUG 定义）
#include <math.h>
#include <stdio.h>

// ============ 全局数据接口 ============
BalanceData_t balance_data = {0};

// ============ 调试配置 ============
static DebugMode_t debug_mode = DEBUG_MODE_WIRED;

// ============ 函数声明 ============
void Balance_Init(void);
void Balance_SetDebugMode(DebugMode_t mode);
void Balance_SendTelemetry(void);


/**
 * @brief 核心控制律：读取->计算->输出 (原子操作)
 * 此函数将被定时器中断调用，严禁在此函数中进行耗时操作 (如 printf, OLED, 延时)
 */
void Balance_Control_Loop(void) {
    int16_t ax, ay, az, gx, gy, gz;
    MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);
	
		gy -= g_gyro_y_offset;
		GY = gy;	

    
    // 互补滤波 (保持浮点，F103 较慢但 5ms 内通常能完成)
    angleAcc = -atan2((float)ax, (float)az) / 3.14159f * 180.0f;
		angleAcc -= g_angleAcc_offset;
    angleGyro = angle + ((float)gy / 32768.0f * 2000.0f * 0.005f); // 0.005s = 5ms
    float alpha = 0.01f;
    angle = alpha * angleAcc + (1.0f - alpha) * angleGyro;

  
    // 倒地保护
    if(angle > 50.0f || angle < -50.0f) {
        run_flag = false;
        // 立即停机
//				TIM2->CCR1 = 0;//直接操作寄存器能快点，但是会破坏宏进行的引脚管理，一旦引脚改变，这里没改，就出错了。
//				TIM2->CCR2 = 0; //先用函数吧，确定引脚不变后，可改为寄存器操作。
        Motor_SetSpeed(MOTOR_LEFT, 0);
        Motor_SetSpeed(MOTOR_RIGHT, 0);
        PID_reset(&anglePID);
				PID_reset(&speedPID);
				PID_reset(&turnPID);
//				GPIOC->BSRR = GPIO_Pin_13;//直接操作寄存器，省去函数调用
//				LED1_OFF();
        
        // 更新全局数据
        balance_data.run_flag = false;
        balance_data.pwm_out = 0;
        return; 
    }

    if (!run_flag) {
        Motor_SetSpeed(MOTOR_LEFT, 0);
        Motor_SetSpeed(MOTOR_RIGHT, 0);
        PID_reset(&anglePID);
				PID_reset(&speedPID);
				PID_reset(&turnPID);
//				GPIOC->BSRR = GPIO_Pin_13;
        
        balance_data.run_flag = false;
        balance_data.pwm_out = 0;
        return;
    }

    // 2. PID 计算
//    anglePID.target = 0.0f; // 直立目标为 0
    anglePID.actual = angle;
    PID_update(&anglePID);

    // 3. 左右轮分配 (假设 dif_PWM 是全局变量，由速度环或遥控更新)
    // 注意：dif_PWM 的更新也需要注意原子性，如果它在主循环改，这里读，可能读到一半。
    // 对于 F103 单核，简单变量读写通常是原子的，但最好关中断保护或保持一致性
    left_PWM = -(anglePID.out) + (dif_PWM / 2);
    right_PWM = -anglePID.out - (dif_PWM / 2);

    // 4. 硬限幅
    if (left_PWM > anglePID.out_max) left_PWM = anglePID.out_max;
    if (left_PWM < anglePID.out_min) left_PWM = anglePID.out_min;
    if (right_PWM > anglePID.out_max) right_PWM = anglePID.out_max;
    if (right_PWM < anglePID.out_min) right_PWM = anglePID.out_min;

    // 6. 输出
    Motor_SetSpeed(MOTOR_LEFT, left_PWM);
    Motor_SetSpeed(MOTOR_RIGHT, right_PWM);

    // ============ 更新全局调试数据 ============
    balance_data.angle = angle;
    balance_data.angleAcc = angleAcc;
    balance_data.angleGyro = angleGyro;
    balance_data.gyro = gy;
    balance_data.pwm_out = anglePID.out;
    balance_data.run_flag = run_flag;
    balance_data.target_angle = 0.0f;  // 可扩展为速度环给定
}

/**
 * @brief 初始化平衡控制模块
 */
void Balance_Init(void)
{
    debug_mode = CURRENT_DEBUG_MODE;
    balance_data.run_flag = false;
    balance_data.target_angle = 0.0f;
}

/**
 * @brief 设置调试模式
 */
void Balance_SetDebugMode(DebugMode_t mode)
{
    debug_mode = mode;
}

/**
 * @brief 发送遥测数据（在主循环中调用）
 * 
 * 使用阻塞式发送，确保数据完整性
 */
void Balance_SendTelemetry(void)
{
    static uint32_t counter = 0;
    static uint32_t timestamp = 0;
    counter++;
    timestamp += 10;
    
    // ✅ 改为每秒发送一次（1Hz），避免与命令响应冲突
    // 原来 200ms(5Hz) -> 现在 1000ms(1Hz)
    if (counter % 100 == 0)
    {
        // 关中断，确保发送过程不被 TIM1 打断
        __disable_irq();
        
        Serial_Printf(USART_DEBUG, "%lu,%.2f,%.2f,%.1f\r\n",
            (unsigned long)timestamp,
            balance_data.angle,
            balance_data.gyro,
            balance_data.pwm_out);
        
        // 等待发送完成（TXE 标志）
        while (USART_GetFlagStatus(USART_DEBUG, USART_FLAG_TXE) == RESET);
        
        // 开中断
        __enable_irq();
    }
}

/**
 * @brief 处理串口接收到的 PID 参数命令（在主循环中调用）
 * 
 * 功能：
 * 1. 检查是否有待应用的 PID 参数更新
 * 2. 原子性地应用新参数
 * 3. 发送确认响应
 */
void Control_ProcessCommands(void)
{
    // ✅ 从 RX 环形缓冲区读取字节并处理
    int data;
    while ((data = Serial_ReadByte(USART1)) != -1) {
        SerialCmd_ProcessByte((uint8_t)data);
    }
    
    // 应用待更新的 PID 参数（双缓冲机制保证原子性）
    SerialCmd_ApplyPendingParams();
}


