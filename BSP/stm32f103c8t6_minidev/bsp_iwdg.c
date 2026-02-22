#include "bsp_iwdg.h" 	
#include "system_config.h"
#include "OLED.h"
#include "OLED_Fonts.h"
/**
 * @brief  Initializes the Independent Watchdog (IWDG) with the specified maximum timeout time.
 * @param  MaxTime: The maximum timeout time in milliseconds.
 *         This parameter must be within the range of 40 to 26214 milliseconds.
 * @retval None
 */
void MYIWD_Init(uint16_t MaxTime) {
    // 初始化重装载时间变量
    float ReloadTime = 0.0f;

    // 预分频系数数组，实际值为 4, 8, 16, 32, 64, 128, 256
    uint8_t prescaler_values[] = {1, 2, 4, 8, 16, 32, 64};

    // 对应的预分频系数枚举值
    uint8_t prescalers[] = {
        IWDG_Prescaler_4, IWDG_Prescaler_8, IWDG_Prescaler_16,
        IWDG_Prescaler_32, IWDG_Prescaler_64, IWDG_Prescaler_128,
        IWDG_Prescaler_256
    };

    // 解除看门狗寄存器的写保护
    IWDG_WriteAccessCmd(IWDG_WriteAccess_Enable);

    // 根据 MaxTime 选择合适的预分频系数
    for (int i = 0; i < 7; i++) {
        // 计算当前预分频系数下的最大时间
        if (MaxTime < 409.6 * prescaler_values[i]) {
            // 设置预分频系数
            IWDG_SetPrescaler(prescalers[i]);
            // 计算重装载值
            // 公式: ReloadTime = MaxTime / (TLSI * PR) - 1
            // 其中 TLSI = 0.025（1/40）, PR = prescaler_values[i] * 4
            ReloadTime = MaxTime / 0.025 / (prescaler_values[i] * 4) - 1;
            break;
        }
    }

    // 检查重装载值是否在有效范围内
    if (ReloadTime <= 4096) {
        // 设置重装载值
        IWDG_SetReload((u16)ReloadTime);
    }

    // 手动喂狗，重新加载计数器
    IWDG_ReloadCounter();//这是喂狗代码，应在main函数的while(1)循环的合适位置放置，确保及时喂狗

    // 启动看门狗
    IWDG_Enable();
}
/**
 * @brief  Displays a reset message on the OLED screen.
 * @param  message: The message to display.
 * @retval None
 */
void Display_Reset_Message(const char* message,const char* errortype) {
    // 显示消息
    OLED_ShowChinese(66, 0, message, OLED_12X12_FULL);
	OLED_ShowChinese(66, 18, errortype, OLED_12X12_FULL);
    OLED_Update();
    Delay_ms(500);
    // 清除消息
	OLED_ClearArea(66,0,127-65,32);
	OLED_Update();
    Delay_ms(200);
    // 再次显示消息
    OLED_ShowChinese(66, 0, message, OLED_12X12_FULL);
	OLED_ShowChinese(66, 18, errortype, OLED_12X12_FULL);
    OLED_Update();
    Delay_ms(500);
    // 再次清除消息
	OLED_ClearArea(66,0,127-65,32);
	OLED_Update();
    Delay_ms(200);
	
}
/**
 * @brief 检查并显示系统复位原因
 *
 * 支持三种复位场景：
 * 1. 看门狗超时复位（主循环卡死，未及时喂狗）→ 通过 RCC 标志检测
 * 2. 软件主动复位（调用 NVIC_SystemReset() 前已记录错误）→ 通过 ErrorTime[] 日志推断
 * 3. 正常上电或手动复位 → 无错误日志且非看门狗复位
 */
void Check_Reset_Way(void) {
    // 从存储加载复位计数
    Reset_Count = Store_Data[RESET_TIMERS_STORE_IDX];

    // 显示统一启动图标
    OLED_ShowImageArea(0, 0, 63, 63, 0, 0, 64, 64, Sherry);

    // ✅ 优先检查：是否为看门狗复位（主循环卡死）
    if (RCC_GetFlagStatus(RCC_FLAG_IWDGRST) == SET) {
        // 看门狗复位：说明系统卡死，ErrorType() 未被执行
        Reset_Count++;
        Store_Data[RESET_TIMERS_STORE_IDX] = Reset_Count;
        Store_Save();

        Display_Reset_Message("看门狗复位", "主循环卡死");

        // 清除复位标志，避免下次误判
        RCC_ClearFlag();
        return;
    }

    // ✅ 其次检查：是否有软件主动记录的错误（NVIC_SystemReset 前调用 ErrorType）
    u8 log_index = Store_Data[ERROR_LOG_STORE_IDX];
    u8 err_type = 0;

    // 从最新写入位置的前一个开始回溯，找最近的有效错误
    for (u8 i = 0; i < ERROR_TIME_ARRAY_SIZE; i++) {
        u8 idx = (log_index + ERROR_TIME_ARRAY_SIZE - 1 - i) % ERROR_TIME_ARRAY_SIZE;
        if (ErrorTime[idx].errortype != 0) {
            err_type = ErrorTime[idx].errortype;
            break;
        }
    }

    if (err_type != 0) {
        // 软件主动复位，已有错误上下文
        Reset_Count++;
        Store_Data[RESET_TIMERS_STORE_IDX] = Reset_Count;
        Store_Save();

        const char* title = "异常复位";
        const char* msg = "未知错误";

        switch (err_type) {
            case ENV_COMM_DATA_TRANSMISSION_FAILURE:
                msg = "无法发送！";
                break;
            case ENV_COMM_DATA_RECEPTION_FAILURE:
                msg = "无法接收！";
                break;
            case ENV_SENSOR_DUST_ANOMALY:
                msg = "扬尘异常！";
                break;
            case ENV_SENSOR_NOISE_ANOMALY:
                msg = "噪音异常！";
                break;
            default:
                msg = "其他错误";
                break;
        }
        Display_Reset_Message(title, msg);
        return;
    }

    // ✅ 最后：无看门狗标志 + 无错误日志 → 视为正常启动
    Display_Reset_Message("系统启动", " ");
}



/*如果使用了独立看门狗复位，必须在main函数之中，while(1)循环之前加上下面的if判断并清除标志位，只有两句便不封装了。
	if(RCC_GetFlagStatus(RCC_FLAG_IWDGRST) == SET)//如果复位是由看门狗产生的
	{
	
		RCC_ClearFlag();//清除标志位，该标志位必须软件清除！
	}

*/
