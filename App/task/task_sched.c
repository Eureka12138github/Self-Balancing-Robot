/**
 * @file task_sched.c
 * @brief 基于时间片轮转 + DMA事件驱动的轻量级任务调度系统
 *
 * 本模块实现任务：
 * 定时周期任务（由 SysTick 驱动 TaskSchedule 更新）
 */

/* ======================== 头文件包含 ======================== */
#include "task_sched.h"
#include "system_config.h"
#include <stdbool.h>
#include "key.h"
#include "hall_encoder.h"
#include "MPU6050.h"
#include "LED.h"
#include "motor.h"
#include <math.h>
/* ======================== 宏定义 ======================== */
// 任务数组大小计算
#define TASK_NUM_MAX        (sizeof(TaskComps) / sizeof(TaskComps[0]))  ///< 定时任务数量

// 定时任务周期（单位：ms）
#define KEY_SCAN_INTERVEL_MS           4   ///< 按键扫描周期 4*5 = 20ms
#define HALL_ENCODER_GET_INTERVEL_MS           8   ///< 霍尔编码器增量获取周期


/* ======================== 类型定义 ======================== */

/**
 * @brief 任务调度结构体（时间驱动）
 */
typedef struct {
    bool run;                   ///< 是否可执行
    uint16_t TimCount;          ///< 当前计数值
    uint16_t TimeRload;         ///< 重载值（周期）
    void (*pTaskFunc)(void);    ///< 任务函数指针
} TaskComps_t;




/* ======================== 定时任务函数 ======================== */

void Hall_Encoder_Test(void) {
		g_rp_value3 = Hall_Encoder_Get(ENCODER_LEFT);
		g_rp_value4 = Hall_Encoder_Get(ENCODER_RIGHT);

}

//void Angle_Get(void) {
//		MPU6050_GetData(&AX, &AY, &AZ, &GX, &GY, &GZ);
//		angleAcc = -atan2(AX,AZ) / 3.14159 * 180;  
//		angleGyro = angle + GY / 32768.0 * 2000*0.001;//满量程下正负2000°/s，这里是在进行缩放，需要看MPU6050手册才可理解
//		float alpha = 0.1;//此值与与计算周期有关，需适当调整
//		angle = alpha * angleAcc + (1 - alpha) * angleGyro;//这里是互补滤波，基于acc抑制gyro的零漂
//    if(angle > 40 || angle < -40) { // 阈值设小一点，40度基本救不回来了
//        run_flag = false;
//        LED1_OFF();
//        
//        // 【关键】立即物理停机，不要等 PID 任务
//        Motor_SetSpeed(MOTOR_LEFT, 0);
//        Motor_SetSpeed(MOTOR_RIGHT, 0);
//        
//        // 【关键】清零 PID 状态，防止积分饱和
//        PID_reset(&anglePID); // 
//    }
//}

//// 全局变量保存上一次的值，用于斜坡限制
//static int16_t prev_left_pwm = 0;
//static int16_t prev_right_pwm = 0;

//void Angle_PID(void) {
//    // 1. 安全检查：如果标志位无效，直接停机并清零
//    if (!run_flag) {
//        Motor_SetSpeed(MOTOR_LEFT, 0);
//        Motor_SetSpeed(MOTOR_RIGHT, 0);
//        // 重要：这里最好调用 PID_Reset(&anglePID) 清零积分项
//        prev_left_pwm = 0;
//        prev_right_pwm = 0;
//        return;
//    }

//    // 2. PID 计算
//    anglePID.actual = angle;
//    PID_update(&anglePID);
//    
//    // 3. 分配左右轮
//    int32_t raw_left = -anglePID.out + (dif_PWM / 2);
//    int32_t raw_right = -anglePID.out - (dif_PWM / 2);

//    // 4. 限幅 (硬限制)
//    if (raw_left > anglePID.out_max) raw_left = anglePID.out_max;
//    if (raw_left < anglePID.out_min) raw_left = anglePID.out_min;
//    if (raw_right > anglePID.out_max) raw_right = anglePID.out_max;
//    if (raw_right < anglePID.out_min) raw_right = anglePID.out_min;

//    // 5. 【核心】斜坡限制 (软限制，防止突变)
//    // 每次最多变化 30-50 个单位
//    int16_t max_step = 40; 
//    
//    int16_t target_left = (int16_t)raw_left;
//    int16_t target_right = (int16_t)raw_right;

//    if (target_left > prev_left_pwm + max_step) target_left = prev_left_pwm + max_step;
//    else if (target_left < prev_left_pwm - max_step) target_left = prev_left_pwm - max_step;

//    if (target_right > prev_right_pwm + max_step) target_right = prev_right_pwm + max_step;
//    else if (target_right < prev_right_pwm - max_step) target_right = prev_right_pwm - max_step;

//    // 6. 输出
//    Motor_SetSpeed(MOTOR_LEFT, target_left);
//    Motor_SetSpeed(MOTOR_RIGHT, target_right);

//    // 7. 更新历史值
//    prev_left_pwm = target_left;
//    prev_right_pwm = target_right;
//}


/* 在 task_sched.c 或专门的 control.c 中 */

// 全局变量用于斜坡限制 (保持不变)
static int16_t prev_left_pwm = 0;
static int16_t prev_right_pwm = 0;

/**
 * @brief 核心控制律：读取->计算->输出 (原子操作)
 * 此函数将被定时器中断调用，严禁在此函数中进行耗时操作(如printf, OLED, 延时)
 */
void Balance_Control_Loop(void) {
    int16_t ax, ay, az, gx, gy, gz;
    MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz); 
    
    // 互补滤波 (保持浮点，F103较慢但5ms内通常能完成)
    float angleAcc = -atan2((float)ax, (float)az) / 3.14159f * 180.0f;
    static float angle_last = 0.0f;
    float angleGyro = angle_last + ((float)gy / 32768.0f * 2000.0f * 0.005f); // 0.005s = 5ms
    float alpha = 0.1f;
    float current_angle = alpha * angleAcc + (1.0f - alpha) * angleGyro;
    angle_last = current_angle; // 更新用于下次积分
    
    // 倒地保护
    if(current_angle > 40.0f || current_angle < -40.0f) {
        run_flag = false;
        // 立即停机
        Motor_SetSpeed(MOTOR_LEFT, 0);
        Motor_SetSpeed(MOTOR_RIGHT, 0);
        PID_reset(&anglePID);
        prev_left_pwm = 0;
        prev_right_pwm = 0;
        // 可以在这里置位一个标志，让主循环去关灯
        return; 
    }

    if (!run_flag) {
        Motor_SetSpeed(MOTOR_LEFT, 0);
        Motor_SetSpeed(MOTOR_RIGHT, 0);
        prev_left_pwm = 0;
        prev_right_pwm = 0;
        return;
    }

    // 2. PID 计算
    anglePID.target = 0.0f; // 直立目标为0
    anglePID.actual = current_angle;
    PID_update(&anglePID);

    // 3. 左右轮分配 (假设 dif_PWM 是全局变量，由速度环或遥控更新)
    // 注意：dif_PWM 的更新也需要注意原子性，如果它在主循环改，这里读，可能读到一半。
    // 对于 F103 单核，简单变量读写通常是原子的，但最好关中断保护或保持一致性
    int32_t raw_left = -anglePID.out + (dif_PWM / 2);
    int32_t raw_right = -anglePID.out - (dif_PWM / 2);

    // 4. 硬限幅
    if (raw_left > anglePID.out_max) raw_left = anglePID.out_max;
    if (raw_left < anglePID.out_min) raw_left = anglePID.out_min;
    if (raw_right > anglePID.out_max) raw_right = anglePID.out_max;
    if (raw_right < anglePID.out_min) raw_right = anglePID.out_min;

    // 5. 斜坡限制 (防止突变)
    int16_t max_step = 40; 
    int16_t target_left = (int16_t)raw_left;
    int16_t target_right = (int16_t)raw_right;

    if (target_left > prev_left_pwm + max_step) target_left = prev_left_pwm + max_step;
    else if (target_left < prev_left_pwm - max_step) target_left = prev_left_pwm - max_step;

    if (target_right > prev_right_pwm + max_step) target_right = prev_right_pwm + max_step;
    else if (target_right < prev_right_pwm - max_step) target_right = prev_right_pwm - max_step;

    // 6. 输出
    Motor_SetSpeed(MOTOR_LEFT, target_left);
    Motor_SetSpeed(MOTOR_RIGHT, target_right);

    // 7. 更新历史
    prev_left_pwm = target_left;
    prev_right_pwm = target_right;
    
    // 更新全局 angle 供其他任务参考
    angle = current_angle;
}



/* ======================== 任务表定义 ======================== */

/** @brief 定时任务表 */
static TaskComps_t TaskComps[] = {
	
	{0, KEY_SCAN_INTERVEL_MS,      KEY_SCAN_INTERVEL_MS, Key_Scan},
	{0, HALL_ENCODER_GET_INTERVEL_MS,      HALL_ENCODER_GET_INTERVEL_MS, Hall_Encoder_Test},


};


/* ======================== 调度器实现 ======================== */

/**
 * @brief 定时任务调度器
 *
 * 更新所有定时任务的时间计数器。
 * 当计数归零时，重载计数值并标记任务为可执行（run = 1）。
 */
void TaskSchedule(void) {
    for (u8 i = 0; i < TASK_NUM_MAX; i++) {
        if (TaskComps[i].TimCount > 0) {
            TaskComps[i].TimCount--;

            // 时间到，重载并标记可执行
            if (TaskComps[i].TimCount == 0) {
                TaskComps[i].TimCount = TaskComps[i].TimeRload;
                TaskComps[i].run = 1;  // 所有任务默认可执行
            }
        }
    }
}

/**
 * @brief 定时任务执行器
 *
 * 遍历任务表，执行所有标记为可执行的任务。
 */
void TaskHandler(void) {
    for (u8 i = 0; i < TASK_NUM_MAX; i++) {
        if (TaskComps[i].run && TaskComps[i].pTaskFunc) {
            TaskComps[i].run = 0;
            TaskComps[i].pTaskFunc();
        }
    }
}


/* ======================== 系统辅助函数 ======================== */

// ...
