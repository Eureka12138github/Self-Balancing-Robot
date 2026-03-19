// system_config.c
#include "system_config.h"

// ⚠️ 此部分应为系统的整体配置，可能存在某些敏感信息，需在此定义然后外部调用，避免硬编码

//ESP8266
const char* ESP8266_SNTP_CONFIG = "AT+CIPSNTPCFG=1,8,\"ntp1.aliyun.com\"\r\n";

//一些全局变量
uint16_t Reset_Count = 0;

int16_t g_rp_value1 = 0;
int16_t g_rp_value2 = 0;
int16_t g_rp_value3 = 0;
int16_t g_rp_value4 = 0;

int16_t F_Speed = 0;
int16_t Location = 0;
bool run_flag = false;

int16_t AX,AY,AZ,GX,GY,GZ;
float angleAcc;
float angleGyro;
float angle;

PID_t anglePID = {
	.Kp = 3.6,
	.Ki = 0,
	.Kd = 18,
	.out_max = 100,
	.out_min = -100,
	.out_offset = 5,
//	.errorInt_max = 500,
//	.errorInt_min = -500,
	
};

PID_t speedPID = {
	.Kp = 1.35,
	.Ki = 0.006,
	.Kd = 0,
	.out_max = 20,//这里表示角度的倾斜范围，即用角度控制速度
	.out_min = -20,
//	.errorInt_max = 150,
//	.errorInt_min = -150,	
};

PID_t turnPID = {
	.Kp = 0,
	.Ki = 0,
	.Kd = 0,
	.out_max = 50,//这里表示角度的倾斜范围，即用角度控制速度
	.out_min = -50,
//	.errorInt_max = 20,
//	.errorInt_min = -20,	
};

int16_t left_PWM;//左轮PWM
int16_t right_PWM;
int16_t ave_PWM;//平均PWM
int16_t dif_PWM;//差分PWM

int16_t g_gyro_y_offset;
float g_angleAcc_offset;

float left_speed,right_speed;
float ave_speed,dif_speed;


