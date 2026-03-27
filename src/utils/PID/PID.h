#ifndef PID_H
#define PID_H
#include <stdint.h>                 // Device header
#include <stdbool.h>

typedef struct {
	float target;
	float actual_1; // 实际（上一次）
	float actual;   // 实际（当前）
	float out;      // PID 运算结果
	float Kp;       // 比例系数
	float Ki;       // 积分系数
	float Kd;       // 微分系数
	float error0;   // 本次误差
	float error1;   // 上次误差
	float errorInt_max; // 积分上限
	float errorInt_min; // 积分下限
	float errorInt;     // 积分累积
	float out_max;      // 输出上限
	float out_min;      // 输出下限
	float out_offset;   // 输出偏移
	
	// ============ 高级优化参数 ============
	bool use_incomplete_diff;  // 是否启用不完全微分
	float diff_filter_coef;    // 微分滤波系数 (0~1, 越小滤波越强)
	float diff_state;          // 微分滤波状态变量
	bool use_variable_int;     // 是否启用变速积分
	float int_slow_threshold;  // 慢积分阈值
	float int_slow_coef;       // 慢积分系数 (0~1)
	
	// ============ 调试与原子更新支持 ============
	uint32_t version;       // 参数版本号（用于检测变化）
	bool pending_update;    // 待更新标志（双缓冲机制）
	float Kp_new;           // 新 Kp 值（缓冲）
	float Ki_new;           // 新 Ki 值（缓冲）
	float Kd_new;           // 新 Kd 值（缓冲）
}PID_t;

void PID_update(PID_t * pid);
void PID_reset(PID_t *pid);

// ============ 高级优化功能初始化函数 ============
void PID_InitAngleLoop(PID_t *pid, float Kp, float Ki, float Kd);
void PID_InitSpeedLoop(PID_t *pid, float Kp, float Ki, float Kd);
void PID_InitTurnLoop(PID_t *pid, float Kp, float Ki, float Kd);
void PID_SetAngleDiffFilterCoef(PID_t *pid, float coef);
void PID_SetAngleIntThreshold(PID_t *pid, float threshold);

// ============ 参数安全更新接口 ============
void PID_SetParams(PID_t *pid, float Kp, float Ki, float Kd);
void PID_ApplyPendingParams(PID_t *pid);
bool PID_HasPendingUpdate(PID_t *pid);

#endif
