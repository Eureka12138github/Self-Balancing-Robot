#include "bsp_alarm.h" 

void Alarm_Init(void)
{

	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB | RCC_APB2Periph_GPIOC,ENABLE);
	GPIO_InitTypeDef GPIO_InitStructure;//定义结构体变量
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;
	GPIO_InitStructure.GPIO_Pin = Buzzer_IO;//选择需要配置的端口
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;//在输入模式下，这里其实不用配置
	GPIO_Init(GPIOB,&GPIO_InitStructure);//蜂鸣器初始化
	GPIO_SetBits(GPIOB,Buzzer_IO);//默认为高电平，不响
	
	GPIO_InitStructure.GPIO_Mode = GPIO_Mode_Out_PP;				
	GPIO_InitStructure.GPIO_Pin = Led_IO;						
	GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz;				
	GPIO_Init(GPIOC, &GPIO_InitStructure);//LED初始化						
	GPIO_SetBits(GPIOC,Led_IO);	//默认为高电平，不亮
											

}

void Buzzer_Turn(void)
{
	if(GPIO_ReadOutputDataBit(GPIOB,Buzzer_IO) == 0){
		GPIO_SetBits(GPIOB,Buzzer_IO);
	}
	else {
		GPIO_ResetBits(GPIOB,Buzzer_IO);
	}
}	

void Led_Turn(void)
{
	if(GPIO_ReadOutputDataBit(GPIOC,Led_IO) == 0){
		GPIO_SetBits(GPIOC,Led_IO);
	}
	else {
		GPIO_ResetBits(GPIOC,Led_IO);
	}
}
void Alarm_Off_Auto(u8 Tpye)
{
	switch(Tpye){
		case Led : GPIO_SetBits(GPIOC,Led_IO);break;
		case Buzzer : GPIO_SetBits(GPIOB,Buzzer_IO);break;//默认为高电平，不响break;
		case Led_And_Buzzer : GPIO_SetBits(GPIOC,Led_IO);GPIO_SetBits(GPIOB,Buzzer_IO);break;
		default:break;
	}
}	
