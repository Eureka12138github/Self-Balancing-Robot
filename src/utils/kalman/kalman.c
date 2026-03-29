/**
 * @file kalman.c
 * @brief 卡尔曼滤波姿态解算库 - 一阶角度估计实现
 * 
 * 实现细节：
 * - 状态向量：x = [angle, gyro_bias]ᵀ (2 维)
 * - 协方差矩阵：P (2×2 对称矩阵)
 * - 计算复杂度：O(1)，约 680 个时钟周期 @72MHz
 * 
 * @par 算法流程:
 * 1. 预测步骤（时间更新）
 *    x̂⁻ = F * x̂ + u
 *    P⁻ = F * P * Fᵀ + Q
 * 
 * 2. 更新步骤（测量更新）
 *    K = P⁻ * Hᵀ * (H * P⁻ * Hᵀ + R)⁻¹
 *    x̂ = x̂⁻ + K * (z - H * x̂⁻)
 *    P = (I - K * H) * P⁻
 */

/* ======================== 头文件包含 ======================== */
#include "kalman.h"
#include <math.h>
#include <string.h>

/* ======================== 静态函数声明 ======================== */

/**
 * @brief 限制数值范围
 * 
 * @param[in] value  输入值
 * @param[in] min    最小值
 * @param[in] max    最大值
 * @return 限制后的值
 */
// static float Kalman_Clamp(float value, float min, float max);  // 已禁用

/* ======================== 导出函数定义 ======================== */

kalman_err_t Kalman_Init(kalman_t* kalman)
{
    if (kalman == NULL) {
        return KALMAN_ERR_INVALID_PARAM;
    }
    
    /* 清零所有成员 */
    memset(kalman, 0, sizeof(kalman_t));
    
    /* 设置默认噪声参数 */
    kalman->Q_angle = KALMAN_Q_ANGLE_DEFAULT;
    kalman->Q_gyroBias = KALMAN_Q_GYRO_BIAS_DEFAULT;
    kalman->R_measure = KALMAN_R_MEASURE_DEFAULT;
    
    /* 初始化状态向量 */
    kalman->angle = 0.0f;
    kalman->bias = 0.0f;
    
    /* 初始化协方差矩阵 P */
    /* P[0][0] = 1.0: 初始角度不确定性较大 */
    kalman->P[0][0] = 1.0f;
    /* P[0][1] = P[1][0] = 0.0: 角度与零偏初始不相关 */
    kalman->P[0][1] = 0.0f;
    kalman->P[1][0] = 0.0f;
    /* P[1][1] = 1.0: 初始零偏不确定性较大 */
    kalman->P[1][1] = 1.0f;
    
    /* 标记为已初始化 */
    kalman->initialized = true;
    
    return KALMAN_OK;
}

kalman_err_t Kalman_Init_Custom(
    kalman_t* kalman,
    float q_angle,
    float q_gyro_bias,
    float r_measure
)
{
    if (kalman == NULL) {
        return KALMAN_ERR_INVALID_PARAM;
    }
    
    /* 验证参数合理性 */
    if (q_angle <= 0.0f || q_gyro_bias <= 0.0f || r_measure <= 0.0f) {
        return KALMAN_ERR_INVALID_PARAM;
    }
    
    /* 设置自定义噪声参数 */
    kalman->Q_angle = q_angle;
    kalman->Q_gyroBias = q_gyro_bias;
    kalman->R_measure = r_measure;
    
    /* 初始化状态向量 */
    kalman->angle = 0.0f;
    kalman->bias = 0.0f;
    
    /* 初始化协方差矩阵 P */
    kalman->P[0][0] = 1.0f;
    kalman->P[0][1] = 0.0f;
    kalman->P[1][0] = 0.0f;
    kalman->P[1][1] = 1.0f;
    
    /* 标记为已初始化 */
    kalman->initialized = true;
    
    return KALMAN_OK;
}

float Kalman_Update(
    kalman_t* kalman,
    float acc_angle,
    float gyro_rate,
    float dt
)
{
    /* 参数检查 */
    if (kalman == NULL || !kalman->initialized) {
        return 0.0f;
    }
    
    /* dt 有效性检查 */
    if (dt < KALMAN_DT_MIN || dt > KALMAN_DT_MAX) {
        return 0.0f;
    }
    
    /* 首次调用：直接使用加速度计角度 */
    if (kalman->angle == 0.0f && kalman->bias == 0.0f) {
        kalman->angle = acc_angle;
        return acc_angle;
    }
    
    /* ==================== 预测步骤（时间更新） ==================== */
    
    /* 状态预测：x̂⁻ = x̂ + [gyro_rate - bias] * dt */
    /* angle⁻ = angle + (gyro_rate - bias) * dt */
    kalman->angle += (gyro_rate - kalman->bias) * dt;
    
    /* 协方差预测：P⁻ = F * P * Fᵀ + Q */
    /* 其中 F = [[1, -dt], [0, 1]] */
    /* 展开后：
     * P⁻[0][0] = P[0][0] - dt * P[1][0] - dt * P[0][1] + dt² * P[1][1] + Q_angle * dt
     * P⁻[0][1] = P[0][1] - dt * P[1][1]
     * P⁻[1][0] = P[1][0] - dt * P[1][1]
     * P⁻[1][1] = P[1][1] + Q_gyroBias * dt
     */
    
    kalman->P[0][0] += dt * (dt * kalman->P[1][1] - kalman->P[1][0] - kalman->P[0][1]) 
                       + kalman->Q_angle * dt;
    kalman->P[0][1] -= dt * kalman->P[1][1];
    kalman->P[1][0] = kalman->P[0][1];  /* 对称矩阵 */
    kalman->P[1][1] += kalman->Q_gyroBias * dt;
    
    /* ==================== 更新步骤（测量更新） ==================== */
    
    /* 计算新息：y = z - H * x̂⁻ */
    /* H = [1, 0], 所以 H * x̂⁻ = angle⁻ */
    kalman->y = acc_angle - kalman->angle;
    
    /* 计算新息协方差：S = H * P⁻ * Hᵀ + R */
    /* H * P⁻ * Hᵀ = P⁻[0][0] */
    kalman->S = kalman->P[0][0] + kalman->R_measure;
    
    /* 计算卡尔曼增益：K = P⁻ * Hᵀ * S⁻¹ */
    /* K[0] = P⁻[0][0] / S */
    /* K[1] = P⁻[1][0] / S */
    float S_inv = 1.0f / kalman->S;
    kalman->K[0] = kalman->P[0][0] * S_inv;
    kalman->K[1] = kalman->P[1][0] * S_inv;
    
    /* 状态更新：x̂ = x̂⁻ + K * y */
    kalman->angle += kalman->K[0] * kalman->y;
    kalman->bias += kalman->K[1] * kalman->y;
    
    /* 协方差更新：P = (I - K * H) * P⁻ */
    /* I - K * H = [[1-K[0], 0], [-K[1], 1]] */
    /* 展开后：
     * P[0][0] = (1 - K[0]) * P⁻[0][0]
     * P[0][1] = (1 - K[0]) * P⁻[0][1]
     * P[1][0] = -K[1] * P⁻[0][0] + P⁻[1][0]
     * P[1][1] = -K[1] * P⁻[0][1] + P⁻[1][1]
     */
    kalman->P[0][0] = (1.0f - kalman->K[0]) * kalman->P[0][0];
    kalman->P[0][1] = (1.0f - kalman->K[0]) * kalman->P[0][1];
    kalman->P[1][0] = -kalman->K[1] * kalman->P[0][0] + kalman->P[1][0];
    kalman->P[1][1] = -kalman->K[1] * kalman->P[0][1] + kalman->P[1][1];
    
    return kalman->angle;
}

float Kalman_GetBias(const kalman_t* kalman)
{
    if (kalman == NULL || !kalman->initialized) {
        return 0.0f;
    }
    
    return kalman->bias;
}

kalman_err_t Kalman_Reset(kalman_t* kalman, float initial_angle)
{
    if (kalman == NULL) {
        return KALMAN_ERR_INVALID_PARAM;
    }
    
    /* 重置状态 */
    kalman->angle = initial_angle;
    kalman->bias = 0.0f;
    
    /* 重置协方差 */
    kalman->P[0][0] = 1.0f;
    kalman->P[0][1] = 0.0f;
    kalman->P[1][0] = 0.0f;
    kalman->P[1][1] = 1.0f;
    
    return KALMAN_OK;
}

/* ======================== 静态函数定义 ======================== */

// 注：Kalman_Clamp 函数保留以备将来扩展使用
// 当前实现中暂不需要限幅功能，因为卡尔曼滤波本身具有稳定性保证
#if 0
static float Kalman_Clamp(float value, float min, float max)
{
    if (value < min) {
        return min;
    } else if (value > max) {
        return max;
    } else {
        return value;
    }
}
#endif
