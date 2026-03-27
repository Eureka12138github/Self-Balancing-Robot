#include "control.h"
#include "stm32f10x.h"                  // Device Header
#include "system_config.h"
#include "Motor.h"
#include "MPU6050.h"
#include "LED.h"
#include "hall_encoder.h"   // 霍尔编码器接口
#include "usart.h"          // 串口驱动（包含 USART_DEBUG 定义）
#include <math.h>
#include <stdio.h>

/*============================================================================
 *                      静态全局变量（仅本文件可见）
 *============================================================================*/

// ========== 角度计算中间变量 ==========
static float angleAcc = 0.0f;     // 加速度计计算的角度（度）
static float angleGyro = 0.0f;    // 陀螺仪积分的角度（度）
static float angle = 0.0f;        // 互补滤波后的最终角度（度）

// ========== 电机控制输出 ==========
static int16_t left_PWM = 0;      // 左轮 PWM 输出（-100~100）
static int16_t right_PWM = 0;     // 右轮 PWM 输出（-100~100）
static int16_t dif_PWM = 0;       // 差速转向 PWM（-50~50）

// ========== 速度反馈变量 ==========
static float left_speed = 0.0f;   // 左轮速度（转/秒，rps）
static float right_speed = 0.0f;  // 右轮速度（转/秒，rps）
static float ave_speed = 0.0f;    // 平均速度（转/秒，rps）
static float dif_speed = 0.0f;    // 速度差（转/秒，rps）

/*============================================================================
 *                      辅助函数定义
 *============================================================================*/

/**
 * @brief 限幅辅助函数（内联优化）
 * 
 * @param value 待限幅的值
 * @param min 最小值
 * @param max 最大值
 * @return 限幅后的值
 */
static inline int16_t Clamp(int16_t value, int16_t min, int16_t max)
{
    if (value > max) return max;
    if (value < min) return min;
    return value;
}

/*============================================================================
 *                      初始化函数
 *============================================================================*/

/**
 * @brief 平衡控制模块初始化
 * 
 * 功能：
 * 1. 清零全局变量
 * 2. 设置初始状态
 * 3. 确保系统处于安全状态
 * 
 * @note 此函数应在系统启动时调用一次
 */
void Balance_Init(void)
{
    // 电机 PWM 清零
    left_PWM = 0;
    right_PWM = 0;
    dif_PWM = 0;
    
    // 速度变量清零
    left_speed = 0.0f;
    right_speed = 0.0f;
    ave_speed = 0.0f;
    dif_speed = 0.0f;
    
    // 角度变量清零
    angleAcc = 0.0f;
    angleGyro = 0.0f;
    angle = 0.0f;
    
    // 状态标志
    run_flag = false;  // 初始为停止状态，确保安全
}


/**
 * @brief 直立环控制（每 5ms 调用）
 * 
 * 功能：
 * 1. 读取 MPU6050 数据
 * 2. 互补滤波计算角度
 * 3. 直立环 PID 计算
 * 4. 电机 PWM 输出
 * 
 * @note 此函数应被定时器中断周期性调用（5ms）
 */
static void Balance_Control_Loop(void) {
    int16_t ax, ay, az, gx, gy, gz;
    
    // 读取 IMU 数据（带错误检查 - 防御性编程）
    MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);
    
    // 数据有效性检查（防止 I2C 通信故障）
    // 如果所有值都为 0，可能是 I2C 总线挂死或传感器断电
    if (ax == 0 && ay == 0 && az == 0 && gx == 0 && gy == 0 && gz == 0) {
        // 传感器异常，立即停机
        run_flag = false;
        Motor_SetSpeed(MOTOR_LEFT, 0);
        Motor_SetSpeed(MOTOR_RIGHT, 0);
        return;
    }
	
	gy -= g_gyro_y_offset;

    /**
     * @brief 互补滤波融合加速度计和陀螺仪数据
     * 
     * 算法原理:
     * 1. 加速度计角度：θ_acc = atan2(-ax, -az) [弧度]
     *    - 使用 ax 和 az 是因为小车前后倾斜时，AY 轴受重力影响最小
     *    - 负号是因为坐标系定义（向前倾斜为正角度）
     * 
     * 2. 陀螺仪积分：θ_gyro = θ_prev + ω × Δt
     *    - gy: 陀螺仪 Y 轴原始值 [-32768, 32767]
     *    - 量程：±2000°/s (MPU6050 默认)
     *    - 转换：ω = gy / 32768 × 2000 [°/s]
     *    - 积分：Δθ = ω × 0.005 [°] (5ms 周期)
     *    - 优化：预计算 GYRO_SCALE_FACTOR = 2000×0.005/32768 ≈ 0.000305176
     * 
     * 3. 互补滤波：
     *    θ = α × θ_acc + (1-α) × θ_gyro
     *    - α = 0.01: 低频信任加速度计（消除漂移）
     *    - 1-α = 0.99: 高频信任陀螺仪（响应快速变化）
     *    - 截止频率：fc = α / (2π × Δt) ≈ 0.32Hz
     */
    
    // 加速度计计算角度（弧度 → 角度）
    angleAcc = -atan2((float)ax, (float)az) * RAD_TO_DEG;
	angleAcc -= g_angleAcc_offset;  // 零点校准
    
    // 陀螺仪积分角度（5ms 周期）
    angleGyro = angle + (float)gy * GYRO_SCALE_FACTOR;
    
    // 互补滤波融合
    angle = COMPLEMENTARY_FILTER_ALPHA * angleAcc + 
            (1.0f - COMPLEMENTARY_FILTER_ALPHA) * angleGyro;

    /**
     * @brief 倒地保护机制
     * 
     * 当倾斜角度超过安全阈值时，立即停机防止损坏
     * 阈值设置依据：
     * - 50°: 经验值，适合大多数平衡车结构
     * - 过大会导致恢复困难，过小会误触发
     */
    if(angle > ANGLE_FALL_THRESHOLD || angle < -ANGLE_FALL_THRESHOLD) {
        run_flag = false;
        // 立即停机
        Motor_SetSpeed(MOTOR_LEFT, 0);
        Motor_SetSpeed(MOTOR_RIGHT, 0);
        PID_reset(&anglePID);
		PID_reset(&speedPID);
		PID_reset(&turnPID);
		GPIOC->BSRR = GPIO_Pin_13;//直接操作寄存器，省去函数调用
        
        return; 
    }

    if (!run_flag) {
        Motor_SetSpeed(MOTOR_LEFT, 0);
        Motor_SetSpeed(MOTOR_RIGHT, 0);
        PID_reset(&anglePID);
		PID_reset(&speedPID);
		PID_reset(&turnPID);
		GPIOC->BSRR = GPIO_Pin_13;
        
        return;
    }

    // PID 计算
//    anglePID.target = 0.0f; // 直立目标为 0
    anglePID.actual = angle;
    PID_update(&anglePID);

    // 左右轮分配 
    left_PWM = -(anglePID.out) + (dif_PWM / 2);
    right_PWM = -anglePID.out - (dif_PWM / 2);

    // 硬限幅（使用辅助函数）
    left_PWM = Clamp(left_PWM, anglePID.out_min, anglePID.out_max);
    right_PWM = Clamp(right_PWM, anglePID.out_min, anglePID.out_max);

    // 输出
    Motor_SetSpeed(MOTOR_LEFT, left_PWM);
    Motor_SetSpeed(MOTOR_RIGHT, right_PWM);
}

/**
 * @brief 速度环和转向环控制（每 40ms 调用）
 * 
 * 功能：
 * 1. 读取左右轮编码器速度
 * 2. 计算平均速度和速度差
 * 3. 速度环 PID 计算
 * 4. 转向环 PID 计算
 * 
 * @note 此函数应被定时器中断周期性调用（40ms）
 */
static void SpeedAndTurn_Control_Loop(void)
{
    // 读取编码器速度并转换为转/秒 (rps)
    // 转换公式：speed = encoder_counts / (极对数 × 读取周期 × 减速比)
    // 其中：
    //   - MOTOR_POLE_PAIRS = 44 (电机极对数)
    //   - ENCODER_READ_PERIOD = 0.04s (编码器读取周期)
    //   - GEAR_RATIO = 9.27666 (减速比)
    //   - SPEED_CONVERSION_FACTOR = 44 × 0.04 × 9.27666 ≈ 16.326816
    left_speed = Hall_Encoder_Get(ENCODER_LEFT) / SPEED_CONVERSION_FACTOR;
    right_speed = Hall_Encoder_Get(ENCODER_RIGHT) / SPEED_CONVERSION_FACTOR;
    ave_speed = (left_speed + right_speed) / 2.0;
    dif_speed = left_speed - right_speed;
    
    if (run_flag) {
        // 速度环 PID
        speedPID.actual = ave_speed;
        PID_update(&speedPID);
        anglePID.target = speedPID.out;
        
        // 转向环 PID
        turnPID.actual = dif_speed;
        PID_update(&turnPID);
        dif_PWM = turnPID.out;
    }
}

/**
 * @brief 平衡控制主调度函数
 * 
 * 功能：
 * 1. 直立环控制（每 5ms 调用）
 * 2. 速度环 + 转向环控制（每 40ms 调用）
 * 
 * @note 此函数应被定时器中断周期性调用
 * @note 默认定时器周期为 5ms
 */
void Balance_Control_Scheduler(void)
{
    static uint16_t count = 0;
    
    // ========== 直立环控制（每 5ms） ==========
    Balance_Control_Loop();
    
    // ========== 速度环 + 转向环控制（每 40ms） ==========
    count++;
    if (count >= 8) {  // 5ms × 8 = 40ms
        count = 0;
        SpeedAndTurn_Control_Loop();
    }
}

/*============================================================================
 *                      状态观测器接口实现
 *============================================================================*/

void Balance_GetMonitorData(BalanceMonitor_t* monitor) {
    if (monitor == NULL) return;
    
    monitor->angle = angle;
    monitor->angleAcc = angleAcc;
    monitor->left_pwm = left_PWM;
    monitor->right_pwm = right_PWM;
    monitor->dif_pwm = dif_PWM;
    monitor->ave_speed = ave_speed;
    monitor->dif_speed = dif_speed;
}

