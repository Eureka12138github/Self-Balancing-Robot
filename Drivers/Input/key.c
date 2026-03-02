/*-----------------------------------------------使用说明-----------------------------------------------*/
/*
1. 初始化按键：
    在 `main` 函数中调用 `Key_Init` 函数进行按键初始化。这一步确保按键所需的 GPIO 端口和时钟被正确配置。
	
2. 定时器中断配置：
	在 `main` 函数中调用 `Timer_Init` 函数进行定时器初始化。这一步确保检测按键状态所需的定时器中断频率被正确配置。（通常为1ms）
	将此Key.c文件末尾的定时器中断示例函数放到main.c文件末尾。
    （无论定时器中断的频率如何配置，都需确保 `Key_Scan` 函数每隔 20 ms 被调用一次。这一步确保按键状态被定期扫描和更新。）

3. 定义事件数组：
    在 `main` 函数中定义一个 `KeyEventType` 类型的数组，数组大小不可超过 `MAX_KEYS_NUM`（假设 `MAX_KEYS_NUM` 为 4）。例如：
      KeyEventType Event[MAX_KEYS_NUM] = {KEY_EVENT_NONE};
	  这个数组用于存储每个按键的事件状态。

4. 获取按键事件：
    在 `main` 函数的 `while(1)` 循环中调用 `Key_GetEvent` 函数，传入定义的事件数组和数组大小。例如：
      Key_GetEvent(Event, MAX_KEYS_NUM);
	  这一步将按键事件从内部状态数组 `key_states` 复制到传入的数组 `Event` 中，并将 `key_states` 中的事件标志重置为 `KEY_EVENT_NONE`。

5. 根据事件类型做相应操作：
	Event[0]~Event[3]分别为Key1~Key4的状态
	例如：
	if(Event[0] == KEY_EVENT_CLICK){
		//在按键1被单击时执行的操作。。。
	}
	if(Event[2] == KEY_EVENT_TRIPLE_CLICK){
		//在按键3被三击时执行的操作。。。
	}
*/	
/*-----------------------------------------------注意事项-----------------------------------------------*/
/*
1. 事件数组大小：
    确保定义的事件数组大小不超过 `MAX_KEYS_NUM`，以避免数组越界。例如：
    KeyEventType Event[4] = {KEY_EVENT_NONE}; // 正确
    // KeyEventType Event[5] = {KEY_EVENT_NONE}; // 错误，数组大小超过 MAX_KEYS_NUM

2. 事件处理：
    如果不需要使用双击、三击和长按功能，只需判断 `Event[x]` 是否等于 `KEY_EVENT_CLICK`。例如：
      if (Event[0] == KEY_EVENT_CLICK) {
          // 处理单击事件
      }

*/

#include "key.h"
#include "bsp_config.h" 
#include <stddef.h> 

/**
 * @brief 初始化按键
 *
 * 该函数初始化按键所需的 GPIO 端口和时钟。具体步骤包括：
 * 1. 使能 GPIOA 和 GPIOB 的时钟。
 * 2. 配置 GPIOA 和 GPIOB 的端口模式为上拉输入模式。
 * 3. 设置端口速度为 50 MHz（在输入模式下，速度配置实际上不起作用，但为了完整性仍进行配置）。
 *
 * @note
 * - `RCC_APB2PeriphClockCmd` 用于使能 GPIO 时钟。
 * - `GPIO_InitTypeDef` 结构体用于配置 GPIO 端口。
 * - `GPIO_Mode_IPU` 表示上拉输入模式。
 * - `Key1_PIN`, `Key2_PIN`, `Key3_PIN`, `Key4_PIN` 是按键对应的 GPIO 引脚。
 */
void Key_Init(void)
{
    // 使能 GPIOA 和 GPIOB 的时钟
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOB, ENABLE);
    RCC_APB2PeriphClockCmd(RCC_APB2Periph_GPIOA, ENABLE);

    GPIO_InitTypeDef GPIO_InitStructure; // 定义结构体变量

    // 配置 GPIOA 的按键引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入模式
    GPIO_InitStructure.GPIO_Pin = KEY1_PIN | KEY2_PIN; // 选择需要配置的端口
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 在输入模式下，这里其实不用配置
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 配置 GPIOB 的按键引脚
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IPU; // 上拉输入模式
    GPIO_InitStructure.GPIO_Pin = KEY3_PIN | KEY4_PIN; // 选择需要配置的端口
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_50MHz; // 在输入模式下，这里其实不用配置
    GPIO_Init(GPIOB, &GPIO_InitStructure);
}


/* 按键状态机结构体（每个按键独立维护） */
typedef struct {
    KeyState current_state;   // 当前状态
    uint16_t press_counter;   // 按压计时（单位：20ms）
    uint16_t release_counter; // 释放计时（单位：20ms）
    KeyEventType event;       // 触发事件类型
    uint8_t multi_click_step; // 连击计数（0-2对应单击/双击/三击） 
} KeyStateMachine;

/**
 * @brief 获取指定按键的物理状态
 * @param key_id 按键编号（有效范围：1~4）
 * @return uint8_t 处理后的按键状态：
 *                - 1 = 按键按下（物理低电平）
 *                - 0 = 按键释放（物理高电平）
 * @note 底层硬件接口 Read_KeyX_State() 定义为：
 *       - 0 = 按下（低电平有效）
 *       - 1 = 释放（高电平）
 *       此函数对原始值取反，实现逻辑状态统一化
 */
uint8_t GetKeyState(uint8_t key_id) 
{
    uint8_t key_state = 0;  // 临时保存原始硬件状态

    // 根据按键编号读取对应硬件状态
    switch (key_id) {
        case 1: 
            key_state = READ_KEY1_STATE();  // 按键1（GPIO_PIN_6）GPIOA
            break;
        case 2: 
            key_state = READ_KEY2_STATE();  // 按键2（GPIO_PIN_7）GPIOA
            break;
        case 3: 
            key_state = READ_KEY3_STATE();  // 按键3（GPIO_PIN_0）GPIOB
            break;
        case 4: 
            key_state = READ_KEY4_STATE();  // 按键4（GPIO_PIN_1）GPIOB
            break;
        default:
            key_state = 1;  // 无效按键编号按"释放"处理
            break;
    }

    // 逻辑转换：将硬件电平转换为逻辑状态（低电平有效 → 1=按下）
    return !key_state; 
}

/**
 * @brief 按键状态机数组
 *
 * 该数组存储每个按键的状态信息，包括当前状态、按下计数器、释放计数器、事件类型和多击层级。
 * 每个按键的状态机独立管理其状态，确保按键事件的正确处理。
 *
 * @note
 * - 数组大小为 `MAX_KEYS_NUM`，现 `MAX_KEYS_NUM` 为 4。
 * - 每个按键的状态机初始状态为 `KEY_STATE_IDLE`，所有计数器、事件标志初始化为 0 或 `KEY_EVENT_NONE`。
 */
KeyStateMachine key_states[MAX_KEYS_NUM] = {
    {KEY_STATE_IDLE, 0, 0, KEY_EVENT_NONE,0}, // 按键 1 的初始状态
    {KEY_STATE_IDLE, 0, 0, KEY_EVENT_NONE,0}, // 按键 2 的初始状态
    {KEY_STATE_IDLE, 0, 0, KEY_EVENT_NONE,0}, // 按键 3 的初始状态
    {KEY_STATE_IDLE, 0, 0, KEY_EVENT_NONE,0}  // 按键 4 的初始状态
};

/* 按键扫描函数（定时器中断中调用） */
/**
 * @brief 按键扫描状态机处理函数
 * @details 实现4个独立按键的扫描检测，支持单击、双击、三击识别，以及带渐进间隔的长按事件
 * @note 每个按键状态独立处理，使用消抖机制确保可靠性，时间单位基于定时器中断tick
 */
void Key_Scan(void)
{
    /*--------------- 状态持久化数据 ---------------*/
    static uint8_t last_key_state[MAX_KEYS_NUM] = {0}; // 保存各按键上次物理状态(0释放/1按下)

    /*--------------- 遍历处理所有按键 ---------------*/
    for (uint8_t i = 0; i < MAX_KEYS_NUM; i++) {
        // 获取当前物理状态（硬件层状态，需先映射按键索引）
        uint8_t current_state = GetKeyState(i+1); // 假设按键索引从1开始
        
        /*>>>>>>>>>>>>> 状态机核心处理 <<<<<<<<<<<<<*/
        switch (key_states[i].current_state) {
            /*——— 空闲状态：等待按键按下 ———*/
            case KEY_STATE_IDLE:
                // 检测下降沿（物理按下动作）
                if (current_state && !last_key_state[i]) {
                    key_states[i].current_state = KEY_STATE_DEBOUNCE; // 进入消抖状态
                    key_states[i].press_counter = 0;                  // 重置按压计时
                }
                break;

            /*——— 消抖状态：确认有效按下 ———*/
            case KEY_STATE_DEBOUNCE:
                key_states[i].press_counter++; // 消抖计时递增
                
                // 达到消抖判定时间（DEBOUNCE_TICKS约10-20ms）
                if (key_states[i].press_counter >= DEBOUNCE_TICKS) {
                    if (current_state) { 
                        // 持续按下：确认为有效按压
                        key_states[i].current_state = KEY_STATE_PRESSED; 
                        key_states[i].press_counter = 0;  // 重置为长按计时
                    } else { 
                        // 已释放：判定为抖动干扰
                        key_states[i].current_state = KEY_STATE_IDLE; 
                    }
                }
                break;		

            /*——— 按压保持状态：处理长按与释放 ———*/
            case KEY_STATE_PRESSED:
                /* 情况1：按键释放 */
                if (!current_state) { 
                    // 短按判定（未达到长按阈值）
                    if (key_states[i].press_counter < LONG_PRESS_TICKS) {
                        key_states[i].multi_click_step++; // 点击层级递增（1→单击，2→双击...）
                        
                        // 三击达成：立即触发事件并重置
                        if (key_states[i].multi_click_step >= 3) {
                            key_states[i].event = KEY_EVENT_TRIPLE_CLICK;
                            key_states[i].current_state = KEY_STATE_IDLE;
                            key_states[i].multi_click_step = 0; 
                        } 
                        // 未达三击：进入多击等待状态
                        else {
                            key_states[i].current_state = (key_states[i].multi_click_step == 1) 
                                ? KEY_STATE_WAIT_DOUBLE   // 首次释放→等待双击
                                : KEY_STATE_WAIT_TRIPLE;  // 二次释放→等待三击
                            key_states[i].release_counter = 0; // 重置释放计时
                        }
                    } 
                    // 长按后释放：直接复位状态
                    else {
                        key_states[i].current_state = KEY_STATE_IDLE;
                        key_states[i].multi_click_step = 0; // 清除多击计数
                    }
                    key_states[i].press_counter = 0; // 结束本次按压计时
                } 
                /* 情况2：持续按压（处理长按事件） */
                else { 
                    key_states[i].press_counter++; // 长按持续时间递增
                    
                    // 进入长按事件触发阶段（超过长按判定阈值）
                    if (key_states[i].press_counter >= LONG_PRESS_TICKS) {
                        uint32_t current_time = key_states[i].press_counter;
                        uint32_t interval = 0;
                        uint32_t phase_start = 0;

                        /* 渐进式间隔策略 */
                        // 阶段1：<2秒，间隔0.2秒（示例值）
                        if (current_time < PHASE1_END_TICKS) {         
                            phase_start = LONG_PRESS_TICKS;    // 阶段起始时间
                            interval = INTERVAL_PHASE1;         // 本阶段间隔
                        } 
                        // 阶段2：[2,4]秒，间隔0.1秒
                        else if (current_time < PHASE2_END_TICKS) {
                            phase_start = PHASE1_END_TICKS;
                            interval = INTERVAL_PHASE2;
                        } 
                        // 阶段3：>4秒，间隔0.02秒
                        else {
                            phase_start = PHASE2_END_TICKS;
                            interval = INTERVAL_PHASE3;
                        }

                        // 计算当前阶段持续时间
                        uint32_t time_in_phase = current_time - phase_start;
                        
                        // 触发条件：阶段起点或间隔周期点
                        if (time_in_phase == 0 || (time_in_phase % interval == 0)) {
                            key_states[i].event = KEY_EVENT_LONG_PRESS; // 触发长按事件
                        }
                    }
                }
                break;

            /*——— 等待双击状态：检测第二次按下 ———*/
            case KEY_STATE_WAIT_DOUBLE:
                key_states[i].release_counter++; // 释放持续时间递增
                
                // 等待超时（未等到第二次按下）
                if (key_states[i].release_counter >= DOUBLE_CLICK_TICKS) {
                    key_states[i].event = KEY_EVENT_CLICK; // 最终确认为单击
                    key_states[i].current_state = KEY_STATE_IDLE;
                    key_states[i].multi_click_step = 0; 
                } 
                // 检测到再次按下（下降沿）
                else if (current_state && !last_key_state[i]) {
                    key_states[i].current_state = KEY_STATE_DEBOUNCE; // 重新消抖确认
                    key_states[i].press_counter = 0;                 // 重置按压计时
                }
                break;

            /*——— 等待三击状态：检测第三次按下 ———*/
            case KEY_STATE_WAIT_TRIPLE:
                key_states[i].release_counter++; // 释放持续时间递增
                
                // 等待超时（未等到第三次按下）
                if (key_states[i].release_counter >= TRIPLE_CLICK_TICKS) {
                    key_states[i].event = KEY_EVENT_DOUBLE_CLICK; // 确认为双击
                    key_states[i].current_state = KEY_STATE_IDLE;
                    key_states[i].multi_click_step = 0;
                } 
                // 检测到再次按下（下降沿）
                else if (current_state && !last_key_state[i]) {
                    key_states[i].current_state = KEY_STATE_DEBOUNCE; // 重新消抖确认
                    key_states[i].press_counter = 0;
                }
                break;
        } // end switch
        
        /*--------------- 更新状态历史 ---------------*/
        last_key_state[i] = current_state; // 保存当前状态供下次比较
    } // end for
} // end function

/**
 * @brief 获取按键事件
 *
 * 该函数将按键事件从内部状态数组 `key_states` 复制到传入的数组 `Key_Event` 中，
 * 并将 `key_states` 中的事件标志重置为 `KEY_EVENT_NONE`。
 *
 * @param Key_Event 指向存储按键事件的数组的指针。
 * @param Event_Num 要获取的按键事件数量。
 *
 * @note
 * - `Key_Event` 必须是一个有效的数组指针，且 `Event_Num` 必须小于或等于 `MAX_KEYS_NUM`。
 * - 如果 `Key_Event` 为 `NULL` 或 `Event_Num` 超过 `MAX_KEYS_NUM`，函数将直接返回，不进行任何操作。
 * - 该函数会清除 `key_states` 中的事件标志，确保每个事件只被处理一次。（多任务可能不适用，因为可能会出现事件丢失，需修改）
 */
void Key_GetEvent(KeyEventType* Key_Event, u8 Event_Num)
{
    if (Key_Event == NULL || Event_Num > MAX_KEYS_NUM) { // 假设 MAX_KEYS_NUM = 4
        return; // 参数无效，直接返回
    }

    for (u8 i = 0; i < Event_Num; i++) {
        Key_Event[i] = key_states[i].event;
        key_states[i].event = KEY_EVENT_NONE;
    }
}

/**现在按键要配合定时器(1ms)使用！放在主函数后面
	void TIM2_IRQHandler(void)
{
	static uint8_t T0Count1;
	if(TIM_GetITStatus(TIM2,TIM_IT_Update)==SET)
	{
		T0Count1++;
		TIM_ClearITPendingBit(TIM2,TIM_IT_Update);
	}
	
	if(T0Count1>=20)
	{
		T0Count1=0;
		Key_Scan();//这个函数不能出现Delay等耗时语句，否则定时器会出错
	}
}
**/

/**Timer_Init示例：（1ms）
void Timer_Init(void)
{
    // 使能 TIM2 的时钟
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_TIM2, ENABLE);
    // TIM2 的时基单元由内部时钟驱动（系统默认，可不写）
    TIM_InternalClockConfig(TIM2);

    // 定义 TIM_TimeBaseInitTypeDef 结构体变量
    TIM_TimeBaseInitTypeDef TIM_TimeBaseInitStructure;

    // 配置时钟分频系数
    TIM_TimeBaseInitStructure.TIM_ClockDivision = TIM_CKD_DIV1;
    // 设置重复计数器（高级计数器使用，此处为 0）
    TIM_TimeBaseInitStructure.TIM_RepetitionCounter = 0;
    // 设置自动重装载值（ARR），减一是因为预分频器和计数器都有 1 个数的偏差
    TIM_TimeBaseInitStructure.TIM_Period = 10 - 1;
    // 设置计数模式为向上计数
    TIM_TimeBaseInitStructure.TIM_CounterMode = TIM_CounterMode_Up;
    // 设置预分频器，将主频分频为 7200 份
    TIM_TimeBaseInitStructure.TIM_Prescaler = 7200 - 1;
    // 初始化 TIM2 的时基单元
    TIM_TimeBaseInit(TIM2, &TIM_TimeBaseInitStructure);

    // 清除更新中断标志
    TIM_ClearFlag(TIM2, TIM_FLAG_Update);
    // 使能 TIM2 的更新中断
    TIM_ITConfig(TIM2, TIM_IT_Update, ENABLE);

    // 配置 NVIC 中断优先级组
    NVIC_PriorityGroupConfig(NVIC_PriorityGroup_2);

    // 定义 NVIC_InitTypeDef 结构体变量
    NVIC_InitTypeDef NVIC_InitStructure;

    // 设置 TIM2 的中断通道
    NVIC_InitStructure.NVIC_IRQChannel = TIM2_IRQn;
    // 使能 TIM2 的中断通道
    NVIC_InitStructure.NVIC_IRQChannelCmd = ENABLE;
    // 设置抢占优先级
    NVIC_InitStructure.NVIC_IRQChannelPreemptionPriority = 2;
    // 设置子优先级
    NVIC_InitStructure.NVIC_IRQChannelSubPriority = 1;
    // 初始化 NVIC 中断
    NVIC_Init(&NVIC_InitStructure);

    // 使能 TIM2
    TIM_Cmd(TIM2, ENABLE);
}


**/

