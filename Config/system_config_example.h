// system_config.h
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H
#include "stm32f10x.h"                  // Device header
#include "stdbool.h" 
#define Default_Dust_Limit 75		//
#define Default_Noise_Limit (70.0)		//GB 12523-2011

#define LIMIT_SAVED						false
#define Clear_Stored_Data				false	
#define alarm_off						false

// OneNet 设备凭证
extern const char* ONENET_PROID;
extern const char* ONENET_ACCESS_KEY;
extern const char* ONENET_DEVICE_NAME;
extern const char* ESP8266_ONENET_INFO;
extern const char* ESP8266_SNTP_CONFIG;

// Wi-Fi 配置
extern const char* WIFI_CONNECT_CMD;

extern u16 Dust_Limit;
extern float Noise_Limit;
extern u16 Reset_Count;
extern bool Limit_Save;
extern bool Clear_Data;
extern bool Alarm_Off_Manual;



#endif
