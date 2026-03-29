// system_config.c
#include "system_config.h"

// ⚠️ 此部分应为系统的整体配置，可能存在某些敏感信息，需在此定义然后外部调用，避免硬编码

/*============================================================================
 *                      系统级全局变量（跨模块共享）
 *============================================================================*/

// ========== 系统运行状态标志 ==========
bool run_flag = false;

// ========== PID 控制器参数配置 ==========

/** @brief 直立环 PID 参数（角度环） */
PID_t anglePID = {
	// 基础参数
	.Kp = 3.6,
	.Ki = 0,
	.Kd = 18,
	
	// 限幅设置
	.out_max = 100,
	.out_min = -100,
	.out_offset = 5,
	.errorInt_max = 500,
	.errorInt_min = -500,
	
	// 高级优化功能（不完全微分 + 变速积分）
	.use_incomplete_diff = false,
	.diff_filter_coef = 0.1f,        // 滤波系数：0.05~0.2
	.diff_state = 0.0f,
	.use_variable_int = false,
	.int_slow_threshold = 5.0f,      // 阈值：5 度
	.int_slow_coef = 0.3f,           // 慢积分系数：0.2~0.5
};

/** @brief 速度环 PID 参数 */
PID_t speedPID = {
	.Kp = 1.35,
	.Ki = 0.006,
	.Kd = 0,
	.out_max = 20,  // 用角度控制速度
	.out_min = -20,
	.errorInt_max = 150,
	.errorInt_min = -150,
	// 速度环通常不需要高级优化
	.use_incomplete_diff = false,
	.use_variable_int = false,
	.diff_state = 0.0f,
};

/** @brief 转向环 PID 参数 */
PID_t turnPID = {
	.Kp = 0.12,
	.Ki = 0.580,
	.Kd = 0,
	.out_max = 50,  // 用角度控制转向
	.out_min = -50,
	.errorInt_max = 20,
	.errorInt_min = -20,
	// 转向环通常不需要高级优化
	.use_incomplete_diff = false,
	.use_variable_int = false,
	.diff_state = 0.0f,
};

/*============================================================================
 *                      传感器校准参数（跨模块共享）
 *============================================================================*/

/** @brief 陀螺仪 Y 轴零偏（由 UI 菜单校准） */
int16_t g_gyro_y_offset;

/** @brief 加速度计角度零偏（由 UI 菜单校准） */
float g_angleAcc_offset;

/** @brief 卡尔曼滤波器实例（全局共享） */
kalman_t g_kalman;


