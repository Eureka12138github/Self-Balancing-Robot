#include "dht11_drv.h"
#include "bsp_delay.h"
//对于stm32来说，是输出
//因为这类似于单总线，不能将PIN初始化放在一起，分开写，输入配置为浮空，输出为上拉，这样更直观一点
void DHT11_GPIO_Init_OUT(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = Pin_Select;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
}

void DHT11_GPIO_Init_IN(void)
{
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB,ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN_FLOATING;
	GPIO_InitStructure.GPIO_Pin = Pin_Select;
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;
	GPIO_Init(GPIOB,&GPIO_InitStructure);
}

//DHT11初始化 
//返回 true，初始化成功；false，失败；
bool DHT11_Init(void)//初始化要结合DHT11的通讯时序图来理解
{
	DHT11_GPIO_Init_OUT();
	DHT11_High;
	DHT11_Rst();	  
	return DHT11_Check();	
}
//复位DHT11
void DHT11_Rst(void)   
{    
	DHT11_GPIO_Init_OUT();	
	DHT11_Low; 	//拉低DQ
    Delay_ms(25);   //拉低至少18ms
    DHT11_High; 	//DQ=1 
	Delay_us(30);  //主机拉高20~40us
}

//等待DHT11的回应
//返回 false :未检测到DHT11的存在
//返回 true :存在
bool DHT11_Check(void) 	   
{   
	u8 retry=0;
	DHT11_GPIO_Init_IN();
	while (!DHT11_DQ&&retry<100)//判断从机发出 80us 的低电平响应信号是否结束
	{
		retry++;
		Delay_us(1);
	};
	if(retry>=100)return false;//这里指的是如果拉低时间过长表示可能出错了，返回1，表示初始化错误
	else retry=0;
    while (DHT11_DQ&&retry<100)//判断从机发出 80us 的高电平是否结束如结束则主机进入数据接收状态
	{
		retry++;
		Delay_us(1);
	};	 
	if(retry>=100)return false;	    
	return true;
}

//从DHT11读取一个位
//返回值：1/0
u8 DHT11_Read_Bit(void) 			 
{
 	u8 retry=0;
	while(DHT11_DQ&&retry<100)//等待变为低电平
	{
		retry++;
		Delay_us(1);
	}
	retry=0;
	while(!DHT11_DQ&&retry<100)//等待变高电平
	{
		retry++;
		Delay_us(1);
	}
	Delay_us(40);//等待40us，这是一种更为合适的读取电平时机，原来的低电平读取，26~28us太短了，不好读取
	if(DHT11_DQ)return 1;
	else return 0;		   
}

//从DHT11读取一个字节
//返回值：读到的数据
u8 DHT11_Read_Byte(void)    
{        
	u8 i,dat;
	dat=0;
	for (i=0;i<8;i++) 
	{
		dat<<=1; 
		dat|=DHT11_Read_Bit();
  }						    
  return dat;
}

//从DHT11读取一次数据
//temp:温度值(范围:0~50°)
//humi:湿度值(范围:20%~90%)
//返回值：true,正常;false,读取失败
bool DHT11_Read_Data(u16 *temp,u16 *humi)    
{        
 	u8 buf[5];
	u8 i;
	DHT11_Rst();
	if(DHT11_Check())
	{
		for(i=0;i<5;i++)//读取40位数据
		{
			buf[i]=DHT11_Read_Byte();//5*8 = 40
		}
		if((buf[0]+buf[1]+buf[2]+buf[3])==buf[4])
		//数据传送正确时校验和数据等于“8bit湿度整数数据+8bit湿度小数数据+8bi温度整数数据+8bit温度小数数据”
		{
			*humi=buf[0];
			*temp=buf[2];
		}
		
	}else return false;
	return true;	    
}

