#include "PID.h"                                                  
#include <stddef.h>   // 确保包含标准类型定义，如果项目中已有可省略
#include "stm32f10x.h"  // 添加 CMSIS 头文件以使用 __disable_irq/__enableirq
#include <math.h>      // 用于 fabsf

/**
 * @brief 初始化直立环 PID 参数（推荐使用不完全微分 + 变速积分）
 * @param pid: PID 结构体指针
 * @param Kp: 比例系数
 * @param Ki: 积分系数
 * @param Kd: 微分系数
 */
void PID_InitAngleLoop(PID_t *pid, float Kp, float Ki, float Kd)
{
    if (pid == NULL) return;
    
    // 基础参数
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    
    // 限幅设置
    pid->out_offset = 0.0f;
    
    // 不完全微分配置（重点推荐）
    pid->use_incomplete_diff = true;
    pid->diff_filter_coef = 0.1f;        // 滤波系数：0.05~0.2
    pid->diff_state = 0.0f;              // 状态清零
    
    // 变速积分配置
    pid->use_variable_int = true;
    pid->int_slow_threshold = 5.0f;      // 阈值：5 度
    pid->int_slow_coef = 0.3f;           // 慢积分系数：0.2~0.5
    
    // 清零状态变量
    pid->errorInt = 0.0f;
    pid->error0 = 0.0f;
    pid->error1 = 0.0f;
    pid->actual_1 = 0.0f;
    pid->out = 0.0f;
}

/**
 * @brief 初始化速度环 PID 参数
 * @param pid: PID 结构体指针
 * @param Kp: 比例系数
 * @param Ki: 积分系数
 * @param Kd: 微分系数
 */
void PID_InitSpeedLoop(PID_t *pid, float Kp, float Ki, float Kd)
{
    if (pid == NULL) return;
    
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    
    // 速度环通常不需要不完全微分和变速积分
    pid->use_incomplete_diff = false;
    pid->use_variable_int = false;
    pid->diff_state = 0.0f;
    
    pid->errorInt = 0.0f;
    pid->error0 = 0.0f;
    pid->error1 = 0.0f;
    pid->actual_1 = 0.0f;
    pid->out = 0.0f;
}

/**
 * @brief 初始化转向环 PID 参数
 * @param pid: PID 结构体指针
 * @param Kp: 比例系数
 * @param Ki: 积分系数
 * @param Kd: 微分系数
 */
void PID_InitTurnLoop(PID_t *pid, float Kp, float Ki, float Kd)
{
    if (pid == NULL) return;
    
    pid->Kp = Kp;
    pid->Ki = Ki;
    pid->Kd = Kd;
    
    pid->use_incomplete_diff = false;
    pid->use_variable_int = false;
    pid->diff_state = 0.0f;
    
    pid->errorInt = 0.0f;
    pid->error0 = 0.0f;
    pid->error1 = 0.0f;
    pid->actual_1 = 0.0f;
    pid->out = 0.0f;
}

/**
 * @brief 动态调整直立环微分滤波系数
 * @param pid: PID 结构体指针
 * @param coef 滤波系数 (0.01~0.3)
 */
void PID_SetAngleDiffFilterCoef(PID_t *pid, float coef)
{
    if (pid == NULL) return;
    if (coef < 0.01f) coef = 0.01f;
    if (coef > 0.3f) coef = 0.3f;
    pid->diff_filter_coef = coef;
    pid->use_incomplete_diff = true;
}

/**
 * @brief 动态调整变速积分阈值
 * @param pid: PID 结构体指针
 * @param threshold 误差阈值（度）
 */
void PID_SetAngleIntThreshold(PID_t *pid, float threshold)
{
    if (pid == NULL) return;
    if (threshold < 1.0f) threshold = 1.0f;
    if (threshold > 20.0f) threshold = 20.0f;
    pid->int_slow_threshold = threshold;
    pid->use_variable_int = true;
}

void PID_update (PID_t * pid) {
	pid->error1 = pid->error0;
	pid->error0 = pid->target -  pid->actual;
	
	// ============ 变速积分 ============
	float current_Ki = pid->Ki;
	if(pid->use_variable_int) {
		// 误差大时积分慢，误差小时积分快
		if(fabsf(pid->error0) > pid->int_slow_threshold) {
			current_Ki = pid->Ki * pid->int_slow_coef;  // 慢积分
		}
	} else {
		// ✅ 不启用变速积分时，直接使用原始 Ki
		current_Ki = pid->Ki;
	}
	
	// ============ 积分计算 ============
	if(current_Ki != 0) {
		pid->errorInt += pid->error0 * current_Ki;
		if(pid->errorInt > pid->errorInt_max) {
			pid->errorInt = pid->errorInt_max;
		}
		if(pid->errorInt < pid->errorInt_min) {
			pid->errorInt = pid->errorInt_min;
		}
	}else {pid->errorInt = 0;}

    // ============ 微分计算（不完全微分） ============
	float differential = 0.0f;
	if(pid->use_incomplete_diff) {
		// 一阶惯性滤波：y[n] = α*x[n] + (1-α)*y[n-1]
		// x[n] 是原始微分，y[n] 是滤波后微分
		float raw_diff = pid->actual - pid->actual_1;
		pid->diff_state = pid->diff_filter_coef * raw_diff + 
		                  (1.0f - pid->diff_filter_coef) * pid->diff_state;
		differential = -pid->Kd * pid->diff_state;
	} else {
		// 标准微分先行
		// ✅ 关键修复：不启用时也要清零 diff_state，防止累积
		pid->diff_state = 0.0f;
		differential = -pid->Kd * (pid->actual - pid->actual_1);
	}
	
    // ============ PID 输出 ============
	pid->out = pid->Kp * pid->error0 
					 + pid->errorInt  // 注意：这里已经是积分后的值
					 + differential;
					 
	//输出偏移
	if(pid->out > 0) {pid->out += pid->out_offset;}
	if(pid->out < 0) {pid->out -= pid->out_offset;}	
	
	if(pid->out > pid->out_max) {
		pid->out = pid->out_max;
	}
	if(pid->out < pid->out_min) {
		pid->out = pid->out_min;
	}
	
	pid->actual_1 = pid->actual;
	
}

/**
 * @brief 安全设置 PID 参数（双缓冲机制）
 * 
 * 原子性保证：
 * 1. 只写入缓冲字段，不影响当前运行参数
 * 2. 设置 pending_update 标志
 * 3. 在主循环安全时调用 PID_ApplyPendingParams 应用
 * 
 * @param pid: PID 结构体指针
 * @param Kp: 比例系数
 * @param Ki: 积分系数
 * @param Kd: 微分系数
 */
void PID_SetParams(PID_t *pid, float Kp, float Ki, float Kd)
{
    if (pid == NULL) return;
    
    // 写入缓冲区域
    pid->Kp_new = Kp;
    pid->Ki_new = Ki;
    pid->Kd_new = Kd;
    pid->pending_update = true;  // 标记有待应用的更新
    pid->version++;               // 版本号递增
}

/**
 * @brief 应用待更新的 PID 参数（原子操作）
 * 
 * 调用时机：主循环中安全位置（不在中断中）
 * 
 * @param pid: PID 结构体指针
 */
void PID_ApplyPendingParams(PID_t *pid)
{
    if (pid == NULL || !pid->pending_update) return;
    
    // 临界区保护（关中断）
    __disable_irq();
    
    // 原子性复制缓冲参数到实际参数
    pid->Kp = pid->Kp_new;
    pid->Ki = pid->Ki_new;
    pid->Kd = pid->Kd_new;
    
    pid->pending_update = false;  // 清除标志
    
    __enable_irq();
}

/**
 * @brief 检查是否有待更新的参数
 * 
 * @param pid: PID 结构体指针
 * @return true: 有待更新参数 | false: 无待更新参数
 */
bool PID_HasPendingUpdate(PID_t *pid)
{
    if (pid == NULL) return false;
    return pid->pending_update;
}




/**
 * @brief PID 复位函数
 * 
 * 作用：
 * 1. 清零输出，防止电机突然动作。
 * 2. 清零积分项，消除积分饱和（Windup），防止重启后猛冲。
 * 3. 清零误差历史，防止下次计算时微分项出现巨大跳变。
 * 
 * @param pid: PID 结构体指针
 */
void PID_reset(PID_t *pid) {
    if (pid == NULL) return; // 安全检查

    // 1. 立即停止输出
    pid->out = 0.0f;

    // 2. 核心：清零积分累积
    // 这是防止“倒地后扶起，小车突然疯狂旋转”的关键
    pid->errorInt = 0.0f;

    // 3. 核心：清零误差历史
    // 如果不清零，下次运行时 (error0 - error1) 可能会算出一个巨大的虚假微分值
    pid->error0 = 0.0f;
    pid->error1 = 0.0f;
		pid->actual_1 = 0.0f;

}
