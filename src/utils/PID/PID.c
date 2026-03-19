#include "PID.h"                                                 
#include <stddef.h> // 确保包含标准类型定义，如果项目中已有可省略

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
