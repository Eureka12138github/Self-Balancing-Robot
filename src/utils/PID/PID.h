#ifndef PID_H
#define PID_H
#include <stdint.h>                 // Device header

typedef struct {
	float target;
	float actual_1; //实际
	float actual; //实际
	float out; //PID运算	
	float Kp; //比例 
	float Ki;//积分 实测
	float Kd; //微分
	float error0; // 本次误差
	float error1; //上次误差
	float errorInt_max; // 误差累计
	float errorInt_min; // 误差累计
	float errorInt; // 误差累计
	float out_max; //输出最大值
	float out_min; // 输出最小值
	float out_offset; // 输出偏移
	
}PID_t;

void PID_update(PID_t * pid);
void PID_reset(PID_t *pid);


#endif
