#ifndef ERROR_WARNING_LOG_H_
#define ERROR_WARNING_LOG_H_
#include "stm32f10x.h"                  // Device header
#include "bsp_rtc.h"
#include "storage.h"
#include "string.h"
//#define ERROR_TIME_ARRAY_SIZE 		3//(sizeof(ErrorTime)/sizeof(ErrorTime[0]))
#define WARNING_TIME_ARRAY_SIZE 	3//(sizeof(WarningTime)/sizeof(WarningTime[0]))

#define ENV_COMM_DATA_TRANSMISSION_FAILURE    1  // 数据无法发送
#define ENV_COMM_DATA_RECEPTION_FAILURE       2  // 数据无法接收
#define ENV_SENSOR_DUST_ANOMALY               3  // 扬尘数据异常
#define ENV_SENSOR_NOISE_ANOMALY              4  // 噪音数据异常
#define ENV_SENSOR_TEMP_HUMIDITY_ANOMALY      5  // 温湿度数据异常
#define ENV_ALERT_DUST_OVERLIMIT          1  // 扬尘过大
#define ENV_ALERT_NOISE_OVERLIMIT         2  // 噪音过高
#define ENV_ALERT_DUST_NOISE_COMBINED     3  // 扬尘过大且噪音过高

void ErrorType(u8 Type);
void WarningType(u8 Type);
void ReadStoreErrorTime(void);
#endif
