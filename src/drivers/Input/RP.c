#include "stm32f10x.h"                  // Device header
void RP_Init(void)
{
	//开启时钟
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_ADC2,ENABLE);
	RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA,ENABLE);
	
	RCC_ADCCLKConfig(RCC_PCLK2_Div6);//选择6分频，分频后ADCCLK=72MHZ/6=12MHZ，这里为什么这样配置暂时还不理解240513
	GPIO_InitTypeDef GPIO_InitStructure;
	GPIO_InitStructure.GPIO_Mode=GPIO_Mode_AIN;
	/*
	选择模拟输入模式，该模式下GPIO口是无效的，相当于断开GPIO，防止GPIO口的输入对模拟电压
	造成干扰，AIN模式就是ADC的专用模式
	*/
	GPIO_InitStructure.GPIO_Pin=GPIO_Pin_2|GPIO_Pin_3|GPIO_Pin_2|GPIO_Pin_5;
	GPIO_InitStructure.GPIO_Speed=GPIO_Speed_50MHz;
	GPIO_Init(GPIOA,&GPIO_InitStructure);
	//配置ADC
	
	//关于采样时间，若需要更快的转换，则采用小的参数，需稳定则采用大的参数
	ADC_InitTypeDef ADC_InitStructure;
	ADC_InitStructure.ADC_Mode=ADC_Mode_Independent;//选择独立模式
	ADC_InitStructure.ADC_ScanConvMode=DISABLE;//指定扫描模式为单通道
	ADC_InitStructure.ADC_ContinuousConvMode=DISABLE;//指定转换模式为单次模式
	//ADC_InitStructure.ADC_ContinuousConvMode=ENABLE;
	ADC_InitStructure.ADC_ExternalTrigConv=ADC_ExternalTrigConv_None;//触发源选择软件触发
	ADC_InitStructure.ADC_DataAlign=ADC_DataAlign_Right;//数据对齐选择右对齐
	ADC_InitStructure.ADC_NbrOfChannel=1;//通道数目为1，此参数在多通道模式下才真正起作用，单通道默认只有序列1位置有效
	ADC_Init(ADC2,&ADC_InitStructure);
	ADC_Cmd(ADC2,ENABLE);//开启ADC电源
	//下面是ADC校准，这样会使得转换更加精确
	ADC_ResetCalibration(ADC2);//复位校准
	while(ADC_GetResetCalibrationStatus(ADC2)==SET);//若复位校准未完成，则在此等待
	/*ADC_GetResetCalibrationStatus这个函数似乎是用来获取复位校准的标志位的，若在
	校准，则返回值为SET(即1)，否则返回RESET(即0)
	*/
	ADC_StartCalibration(ADC2);//开始校准
	while(ADC_GetCalibrationStatus(ADC2)==SET);//等待校准完成
	//ADC_SoftwareStartConvCmd(ADC1,ENABLE);//在连续转换只需在开始时初始化一次便可
}

uint16_t RP_GetValue(uint8_t n)
{
	if(n == 1) {
		ADC_RegularChannelConfig(ADC2,ADC_Channel_2,1,ADC_SampleTime_55Cycles5);		
	}
	if(n == 2) {
		ADC_RegularChannelConfig(ADC2,ADC_Channel_3,1,ADC_SampleTime_55Cycles5);
	}
	if(n == 3) {
		ADC_RegularChannelConfig(ADC2,ADC_Channel_4,1,ADC_SampleTime_55Cycles5);		
	}
	if(n == 4) {
		ADC_RegularChannelConfig(ADC2,ADC_Channel_5,1,ADC_SampleTime_55Cycles5);		
	}	
	

	ADC_SoftwareStartConvCmd(ADC2,ENABLE);//软件触发转换
	while(ADC_GetFlagStatus(ADC2,ADC_FLAG_EOC)==RESET);	
	//连续转换模式下，此时数据寄存器会连续不断刷新最新的转换结果，所以就无需判断标志位了
	/*
	等待转换完成，等待时间根据前面的配置，计算出大概为5.6us，具体计算方法见江协7-2的29分左右
	ADC_GetFlagStatus(ADC1,ADC_FLAG_EOC)函数用来获取规则组转换完成标志位
	RESET表示还在转换，而SET表示转换已经完成
	*/
	return ADC_GetConversionValue(ADC2);
	/*
	返回转换的结果
	ADC_GetConversionValue(ADC1)是用来获取转换结果的
	获取了结果它会自动将标志位清零
	*/
}
