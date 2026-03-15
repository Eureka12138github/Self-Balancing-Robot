#include "control.h"
#include "stm32f10x.h"                  // Device header
#include "system_config.h"
#include "Motor.h"
#include "MPU6050.h"
#include "LED.h"
#include <math.h>


/**
 * @brief 核心控制律：读取->计算->输出 (原子操作)
 * 此函数将被定时器中断调用，严禁在此函数中进行耗时操作(如printf, OLED, 延时)
 */
void Balance_Control_Loop(void) {
    int16_t ax, ay, az, gx, gy, gz;
    MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);
	
		gy -= g_gyro_y_offset;
		GY = gy;	

    
    // 互补滤波 (保持浮点，F103较慢但5ms内通常能完成)
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
				GPIOC->BSRR = GPIO_Pin_13;//直接操作寄存器，省去函数调用
//				LED1_OFF();
        return; 
    }

    if (!run_flag) {
        Motor_SetSpeed(MOTOR_LEFT, 0);
        Motor_SetSpeed(MOTOR_RIGHT, 0);
				PID_reset(&anglePID);
				GPIOC->BSRR = GPIO_Pin_13;
        return;
    }

    // 2. PID 计算
    anglePID.target = 0.0f; // 直立目标为0
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


}


