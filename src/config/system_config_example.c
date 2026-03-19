// system_config_example.c
#include "system_config_example.h"

// ⚠️ 注意：此为示例文件，请复制为 system_config.c 并填入你的真实信息
// ⚠️ 确保 system_config.c 已加入 .gitignore，切勿提交！

// OneNet 凭证（从 OneNet 控制台获取）
const char* ONENET_PROID        = "YOUR_ONENET_PRODUCT_ID_HERE";
const char* ONENET_ACCESS_KEY   = "YOUR_ONENET_ACCESS_KEY_HERE";
const char* ONENET_DEVICE_NAME  = "YOUR_DEVICE_NAME_HERE";
const char* ESP8266_ONENET_INFO = "AT+CIPSTART=\"TCP\",\"mqtts.heclouds.com\",1883\r\n";


//ESP8266
const char* ESP8266_SNTP_CONFIG = "AT+CIPSNTPCFG=1,8,\"ntp1.aliyun.com\"\r\n";
//const char* ESP8266_SNTP_CONFIG = "AT+CIPSNTPCFG=1,8,\"cn.ntp.org.cn\",\"ntp.sjtu.edu.cn\"\r\n";
	

// Wi-Fi 配置（替换成你的网络）
const char* WIFI_CONNECT_CMD = "AT+CWJAP=\"YOUR_WIFI_SSID_HERE\",\"YOUR_WIFI_PASSWORD_HERE\"\r\n";

//一些全局变量
u16 Dust_Limit = Default_Dust_Limit;
float Noise_Limit = Default_Noise_Limit;
u16 Reset_Count = 0;

bool Limit_Save = LIMIT_SAVED;
bool Clear_Data = Clear_Stored_Data;
bool Alarm_Off_Manual = alarm_off;
