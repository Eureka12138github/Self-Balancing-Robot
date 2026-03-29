/**
 * @file control_kalman_example.c
 * @brief 卡尔曼滤波集成示例代码（参考用，不要编译到项目中）
 * 
 * 本文件展示如何将现有互补滤波替换为卡尔曼滤波
 * 实际使用时，请将这些修改应用到 control.c 中
 */

#include "kalman.h"

// ========== 全局变量声明（已在 system_config.c 中定义） ==========
// extern kalman_t g_kalman;

/**
 * @brief 修改后的 Balance_Control_Loop - 使用卡尔曼滤波
 * 
 * 主要变更：
 * 1. 移除互补滤波相关代码
 * 2. 调用 Kalman_Update 替代互补滤波
 * 3. 陀螺仪零偏由卡尔曼自动估计，无需手动减去 g_gyro_y_offset
 */
static void Balance_Control_Loop_With_Kalman(void)
{
    int16_t ax, ay, az, gx, gy, gz;
    
    // 读取 IMU 数据（保持不变）
    MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);
    
    // 数据有效性检查（保持不变）
    if (ax == 0 && ay == 0 && az == 0 && gx == 0 && gy == 0 && gz == 0) {
        run_flag = false;
        Motor_SetSpeed(MOTOR_LEFT, 0);
        Motor_SetSpeed(MOTOR_RIGHT, 0);
        return;
    }
    
    // ==================== 卡尔曼滤波部分 ====================
    
    // 1. 计算加速度计角度（与原来相同）
    float acc_angle = -atan2f((float)ax, (float)az) * RAD_TO_DEG;
    
    // 2. 陀螺仪角速度（转换为度/秒，不手动减零偏）
    float gyro_rate = gy / 131.0f;  // MPU6050: 2000°/s / 32768 ≈ 1/131
    
    // 3. 卡尔曼滤波融合（替代互补滤波）
    float dt = 0.005f;  // 5ms 采样周期
    angle = Kalman_Update(&g_kalman, acc_angle, gyro_rate, dt);
    
    // 4. （可选）获取估计的陀螺仪零偏用于调试
    // float bias = Kalman_GetBias(&g_kalman);
    // printf("Estimated bias: %.2f deg/s\n", bias);
    
    // ==================== 倒地保护（保持不变） ====================
    if (angle > ANGLE_FALL_THRESHOLD || angle < -ANGLE_FALL_THRESHOLD) {
        run_flag = false;
        Motor_SetSpeed(MOTOR_LEFT, 0);
        Motor_SetSpeed(MOTOR_RIGHT, 0);
        PID_reset(&anglePID);
        PID_reset(&speedPID);
        PID_reset(&turnPID);
        GPIOC->BSRR = GPIO_Pin_13;
        return;
    }
    
    // 运行标志检查（保持不变）
    if (!run_flag) {
        Motor_SetSpeed(MOTOR_LEFT, 0);
        Motor_SetSpeed(MOTOR_RIGHT, 0);
        PID_reset(&anglePID);
        PID_reset(&speedPID);
        PID_reset(&turnPID);
        GPIOC->BSRR = GPIO_Pin_13;
        return;
    }
    
    // ==================== PID 控制（保持不变） ====================
    anglePID.actual = angle;
    PID_update(&anglePID);
    
    // 左右轮分配（保持不变）
    left_PWM = -(anglePID.out) + (dif_PWM / 2);
    right_PWM = -anglePID.out - (dif_PWM / 2);
    
    // 限幅（保持不变）
    left_PWM = Clamp(left_PWM, anglePID.out_min, anglePID.out_max);
    right_PWM = Clamp(right_PWM, anglePID.out_min, anglePID.out_max);
    
    // 输出（保持不变）
    Motor_SetSpeed(MOTOR_LEFT, left_PWM);
    Motor_SetSpeed(MOTOR_RIGHT, right_PWM);
}

/**
 * @brief 初始化函数修改示例
 * 
 * 在 Balance_Control_Init() 中添加卡尔曼初始化
 */
void Balance_Control_Init_With_Kalman(void)
{
    // ... 原有初始化代码（电机、编码器、定时器等） ...
    
    // ========== 新增：卡尔曼滤波器初始化 ==========
    if (Kalman_Init(&g_kalman) != KALMAN_OK) {
        // 初始化失败处理
        Error_Handler();
    }
    
    // （可选）自定义噪声参数
    // Kalman_Init_Custom(&g_kalman, 0.001f, 0.003f, 0.03f);
    
    // ========== 可选：保留快速校准（3 秒） ==========
    /*
    printf("Calibrating for 3 seconds...\n");
    int32_t sum_gyro = 0;
    for (int i = 0; i < 600; i++) {  // 600 * 5ms = 3s
        MPU6050_GetData(NULL, NULL, NULL, NULL, &gy, NULL);
        sum_gyro += gy;
        Delay_ms(5);
    }
    int16_t initial_bias = sum_gyro / 600;
    g_gyro_y_offset = initial_bias;  // 保存用于显示
    
    // 设置卡尔曼初始零偏
    g_kalman.bias = initial_bias / 131.0f;  // 转换为度/秒
    printf("Initial bias: %d (%.2f deg/s)\n", initial_bias, g_kalman.bias);
    */
}

/**
 * @brief 调试输出示例（通过串口打印收敛过程）
 * 
 * 在 main 循环中添加此函数观察卡尔曼性能
 */
void Debug_Kalman_Convergence(void)
{
    static uint32_t counter = 0;
    
    if (++counter % 20 == 0) {  // 每 100ms 打印一次
        float bias = Kalman_GetBias(&g_kalman);
        printf("[%dms] Angle: %.2f°, Bias: %.3f deg/s\n", 
               counter * 5, angle, bias);
    }
}

/* ==================== 对比测试版本 ==================== */

/**
 * @brief 同时运行两种算法（仅用于对比测试）
 * 
 * 临时保留互补滤波，与卡尔曼滤波进行对比
 */
static void Balance_Control_Loop_Compare(void)
{
    int16_t ax, ay, az, gx, gy, gz;
    MPU6050_GetData(&ax, &ay, &az, &gx, &gy, &gz);
    
    // 加速度计角度
    float acc_angle = -atan2f((float)ax, (float)az) * RAD_TO_DEG;
    
    // 陀螺仪角速度
    float gyro_rate = gy / 131.0f;
    float dt = 0.005f;
    
    // 方法 1：互补滤波（原方案）
    static float angle_comp = 0.0f;
    angle_comp = 0.01f * acc_angle + 0.99f * (angle_comp + gyro_rate * dt);
    
    // 方法 2：卡尔曼滤波（新方案）
    float angle_kalman = Kalman_Update(&g_kalman, acc_angle, gyro_rate, dt);
    
    // 通过串口对比输出
    // printf("Comp: %.2f, Kal: %.2f, Diff: %.2f\n", 
    //        angle_comp, angle_kalman, angle_comp - angle_kalman);
    
    // 实际控制使用卡尔曼输出
    angle = angle_kalman;
    
    // ... 后续 PID 控制代码相同 ...
}
