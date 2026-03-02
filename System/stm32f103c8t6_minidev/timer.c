#include "timer.h"
#include "stm32f10x.h"                  // Device header
#include "bsp_config.h"
void Timer2_Init(void)
{
	RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2,ENABLE);//tIM2是APB1总线的外设，所以要开启APB1时钟
	
	TIM_InternalClockConfig(TIM2);//这句表明TIM2的时基单元由内部时钟驱动，系统默认由内部时钟驱动，不写这句也行
	TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;
	TIM_TimeBaseInitStructure.TIM_ClockDivision=TIM_CKD_DIV1;//TIM_ClockDivision与滤波的频率有关，决定是否对内部时钟进行分频，需滤波强一点，就TIM_CKD_DIV2或者TIM_CKD_DIV4
	TIM_TimeBaseInitStructure.TIM_RepetitionCounter=0;//此处高级计数器才用到，故直接给零了
	TIM_TimeBaseInitStructure.TIM_Period=10-1;//ARR,减一是因为预分频器和计数器都有1个数的偏差
	TIM_TimeBaseInitStructure.TIM_CounterMode=TIM_CounterMode_Up;//这里是设置计数器的计数模式的，设置为向上计数
	TIM_TimeBaseInitStructure.TIM_Prescaler=7200-1;//这里是设置预分频器的主频的“切分”，也就是把主频“切”成7200份
	
	TIM_TimeBaseInit(TIM2,&TIM_TimeBaseInitStructure);//初始化通用定时器TIM2的时基单元
	
	TIM_ClearFlag(TIM2,TIM_FLAG_Update);
	TIM_ITConfig(TIM2,TIM_IT_Update,ENABLE);//开启了更新中断到NVIC的通路，这里不太理解，为什么是更新中断？
	NVIC_InitTypeDef NVIC_InitStructure;
	
	NVIC_InitStructure.NVIC_IRQChannel=TIM2_IRQn;//这句是找到TIM2在NVIC中的中断通道，不同类型的中断，在NVIC中的中断名称也不一
	NVIC_InitStructure.NVIC_IRQChannelCmd=ENABLE;//这句是将该通道打通
	NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = TIMER2_PRIO;
	NVIC_InitStructure.NVIC_IRQChannelSubPriority = SUB_PRIO_UNUSED;
	//上面优先级配置我感觉影响不大，因为只有一个中断源
	NVIC_Init(&NVIC_InitStructure);
	TIM_Cmd(TIM2,ENABLE);//这句相当于给TIM2使能
}

/**
 * @brief 定时器中断服务函数的初始化函数，用于产生20ms的定时器中断
 * @param 无
 * @return 无
 */
void Timer4_Init(void)
{
    /*开启时钟*/
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM4, ENABLE);			//开启TIM4的时钟
    
    /*配置时钟源*/
    TIM_InternalClockConfig(TIM4);		//选择TIM4为内部时钟，若不调用此函数，TIM默认也为内部时钟
    
    /*时基单元初始化*/
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;				//定义结构体变量
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;		//时钟分频，选择不分频，此参数用于配置滤波器时钟，不影响时基单元功能
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;	//计数器模式，选择向上计数
    TIM_TimeBaseInitStructure.TIM_Period = 1000 - 1;				//计数周期，即ARR的值 (改为1000)
    TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;				//预分频器，即PSC的值 (保持7200)
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;			//重复计数器，高级定时器才会用到
    TIM_TimeBaseInit(TIM4, &TIM_TimeBaseInitStructure);				//将结构体变量交给TIM_TimeBaseInit，配置TIM4的时基单元	
    
    /*中断输出配置*/
    TIM_ClearFlag(TIM4, TIM_FLAG_Update);						//清除定时器更新标志位
                                                            //TIM_TimeBaseInit函数末尾，手动产生了更新事件
                                                            //若不清除此标志位，则开启中断后，会立刻进入一次中断
                                                            //如果不介意此问题，则不清除此标志位也可
    
    TIM_ITConfig(TIM4, TIM_IT_Update, ENABLE);					//开启TIM4的更新中断
    
    
    /*NVIC配置*/
    NVIC_InitTypeDef NVIC_InitStructure;						//定义结构体变量
    NVIC_InitStructure.NVIC_IRQChannel = TIM4_IRQn;				//选择配置NVIC的TIM4线
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;				//指定NVIC线路使能
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = TIMER4_PRIO;	//指定NVIC线路的抢占优先级为2
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = SUB_PRIO_UNUSED;			//指定NVIC线路的响应优先级为2
    NVIC_Init(&NVIC_InitStructure);								//将结构体变量交给NVIC_Init，配置NVIC外设
    
    /*TIM使能*/
    TIM_Cmd(TIM4, ENABLE);			//使能TIM4，定时器开始运行
}

/*
定时器名称是规定好的，需要在start文件里面的mds里面找
void TIM2_IRQHandler(void)
{
	if(TIM_GetITStatus(TIM2,TIM_IT_Update)==SET)
	{
		Num++;
		TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
	}
}

*/
