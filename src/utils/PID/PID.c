#include "PID.h"                                                  
#include <stddef.h>   // 确保包含标准类型定义，如果项目中已有可省略
#include "stm32f10x.h"  // 添加 CMSIS 头文件以使用 __disable_irq/__enable_irq

void PID_update (PID_t * pid) {
	pid->error1 = pid->error0;
	pid->error0 = pid->target -  pid->actual;
	if(pid->Ki != 0) {
		pid->errorInt += pid->error0;
		if(pid->errorInt > pid->errorInt_max) {
			pid->errorInt = pid->errorInt_max;
		}
		if(pid->errorInt < pid->errorInt_min) {
			pid->errorInt = pid->errorInt_min;
		}
	}else {pid->errorInt = 0;}

	pid->out = pid->Kp * pid->error0 
						 + pid->Ki * pid->errorInt 
//						 + pid->Kd * (pid->error0 - pid->error1);
						 + -pid->Kd * (pid->actual - pid->actual_1);//微分先行
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
