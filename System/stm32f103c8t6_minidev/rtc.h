#ifndef RTC_H
#define RTC_H
#include "stm32f10x.h"
#include "usart.h"
#include "debug_config.h"
#include "stdbool.h"
#include <time.h>
typedef struct MYRTC {
	u16 Year;
	u8 Month;
	u8 Day;
	u8 Hour;
	u8 Minute;
	u8 Second;
	u8 wday;
} MYRTC;
//错误时间信息结构体
typedef struct ErrorTimeStructure {
	MYRTC errortime;
	u8 errortype;
	bool errorshowflag;
} ErrorTimeStructure;
//警报时间信息结构体
typedef struct WarningTimeStructure {
	MYRTC warningtime;
	u8 warningtype;
	bool warningshowflag;
} WarningTimeStructure;

extern MYRTC Time;//这个是实时时间结构体，外部调用访问成员即可获取时间
extern ErrorTimeStructure ErrorTime[];
extern WarningTimeStructure WarningTime[];
//有以下几种错误类型：
//0：无错误
//1：esp8266数据无法发送；
//2：esp8266数据无法接收：
//3：扬尘数据异常；
//4：噪音数据异常；
//5：温湿度数据异常；

void MyRTC_Init(void);
void MyRTC_SetTime(void);
void MyRTC_ReadTime(void);
void ErrorTimeReset(void);
void WarningTimeReset(void);
#endif
/*
函数雏形：
_Bool ESP8266_SNTP_Time(char *cmd, char *res,MYRTC* TimeStructure)
{
	
	实现功能：
			1、AT指令获取SNTP时间；
			2、解析返回的时间字符串并将对应的信息放到TimeStructure结构体中；
例如：执行Usart_SendString(USART2, (unsigned char *)cmd, strlen((const char *)cmd));（发送:AT+CIPSNTPTIME?）
正常情况下，会返回如：

“+CIPSNTPTIME:Sun Apr 20 13:35:18 2025
OK”这样的字符串，在获取返回字符串后，需要解析这个字符串，
解析出来的结果应该是：TimeStructure->Year = 2025;
						TimeStructure->Month = 4;
						TimeStructure->Day = 20;
						TimeStructure->Hour = 13;
						TimeStructure->Minute = 35;
						TimeStructure->Second = 18;
						TimeStructure->wday = 6;
						解析成功后返回0，否则返回1；
						
下面的是参考程序示例：
_Bool ESP8266_SendCmd(char *cmd, char *res)
{
	
	unsigned char timeOut = 200;

	Usart_SendString(USART2, (unsigned char *)cmd, strlen((const char *)cmd));
	
	while(timeOut--)
	{
		if(ESP8266_WaitRecive() == REV_OK)							//如果收到数据
		{
			if(strstr((const char *)esp8266_buf, res) != NULL)		//如果检索到关键词
			{
				ESP8266_Clear();									//清空缓存
				
				return 0;
			}
		}
		
		Delay_ms(10);
	}
	
	return 1;

}						

}
可以看到，这个示例程序只是发送指令并检查是否返回了“OK”，并没有解析字符串，我需要你参考这个程序实现上面我所提到的函数代码功能。
*/
