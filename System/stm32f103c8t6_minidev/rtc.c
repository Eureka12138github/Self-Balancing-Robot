#include "rtc.h"
#include "debug_config.h"
#include <time.h>
//MYRTC Time = {
//	.Year = 2025,
//	.Month = 4,
//	.Day = 24,
//	.Hour = 12,
//	.Minute = 04,
//	.Second = 30,
//	.wday = 4
//};

MYRTC Time = {
	.Year = 2025,
	.Month = 12,
	.Day = 14,
	.Hour = 11,
	.Minute = 40,
	.Second = 00,
	.wday = 0
};

//这些变量主要用于记录错误发生的时间
ErrorTimeStructure ErrorTime[] = {
{{.Year = 1970,.Month = 1,.Day = 1,.Hour = 0,.Minute = 0,.Second = 0,.wday = 4},0,false},
{{.Year = 1970,.Month = 1,.Day = 2,.Hour = 0,.Minute = 0,.Second = 0,.wday = 4},0,false},
{{.Year = 1970,.Month = 1,.Day = 3,.Hour = 0,.Minute = 0,.Second = 0,.wday = 4},0,false},
};

WarningTimeStructure WarningTime[] = {
{{.Year = 1970,.Month = 1,.Day = 1,.Hour = 0,.Minute = 0,.Second = 0,.wday = 4},0,false},
{{.Year = 1970,.Month = 1,.Day = 2,.Hour = 0,.Minute = 0,.Second = 0,.wday = 4},0,false},
{{.Year = 1970,.Month = 1,.Day = 3,.Hour = 0,.Minute = 0,.Second = 0,.wday = 4},0,false},
};

void ErrorTimeReset(void){
	for(u8 i = 0;i<(sizeof(ErrorTime)/sizeof(ErrorTime[0]));i++){
		ErrorTime[i].errortime.Year = 1970;
		ErrorTime[i].errortime.Month = 1;
		ErrorTime[i].errortime.Day = 1;
		ErrorTime[i].errortime.Hour = 0;
		ErrorTime[i].errortime.Minute = 0;
		ErrorTime[i].errortime.Second = 0;
		ErrorTime[i].errortime.wday = 4;
		ErrorTime[i].errortype = 0;
		ErrorTime[i].errorshowflag = false;
	}
}
void WarningTimeReset(void){
	for(u8 i = 0;i<(sizeof(ErrorTime)/sizeof(ErrorTime[0]));i++){
		WarningTime[i].warningtime.Year = 1970;
		WarningTime[i].warningtime.Month = 1;
		WarningTime[i].warningtime.Day = 1;
		WarningTime[i].warningtime.Hour = 0;
		WarningTime[i].warningtime.Minute = 0;
		WarningTime[i].warningtime.Second = 0;
		WarningTime[i].warningtime.wday = 4;
		WarningTime[i].warningtype = 0;
		WarningTime[i].warningshowflag = false;
	}
}
void MyRTC_SetTime(void);

/**
 * @brief  初始化实时时钟(RTC)并配置相关硬件设置【已废弃】
 * 
 * 本函数完成以下功能：
 * 1. 启用PWR和BKP外设时钟以访问备份域
 * 2. 配置LSE(低速外部晶振)作为RTC时钟源
 * 3. 通过备份寄存器判断首次初始化状态，避免系统复位导致时间重置
 * 4. 完成RTC时钟配置、预分频器设置和时间基准初始化
 * 
 * @note 设计特点：
 * - 使用BKP_DR1寄存器存储初始化标志(0xA5A5)
 * - 首次初始化时配置LSE并设置RTC时间基准
 * - 后续复位时保留RTC配置，需VBAT备用电源维持备份域供电
 * - 所有RTC寄存器操作需等待前次操作完成(RTC_WaitForLastTask)
 * 
 * @warning 硬件依赖：
 * - 需要连接32.768kHz低速晶振
 * - 依赖STM32标准外设库的RCC/PWR/BKP/RTC驱动
 * 
 * 工作流程：
 * 1. 使能PWR和BKP时钟，获取备份域写权限
 * 2. 检查BKP_DR1初始化标志：
 *    - 未初始化(标志不匹配)：执行完整初始化流程
 *      a. 启动LSE晶振并等待就绪
 *      b. 选择LSE作为RTC时钟源并启用
 *      c. 配置预分频器(32768-1)生成1Hz时钟
 *      d. 调用MyRTC_SetTime设置初始时间
 *      e. 写入初始化标志到备份寄存器
 *    - 已初始化(标志匹配)：仅使能RTC时钟并同步
 */
//void MyRTC_Init(void)
//{
//    /* 启用电源接口和备份域时钟 */
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
//    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);

//    /* 允许访问备份寄存器 */
//    PWR_BackupAccessCmd(ENABLE);

//    /* 检查是否首次初始化 */
//    if (BKP_ReadBackupRegister(BKP_DR1) != 0XA5A5) 
//    {
//        /* 完整初始化流程 */
//        RCC_LSEConfig(RCC_LSE_ON);  // 启动LSE晶振
//        while (RCC_GetFlagStatus(RCC_FLAG_LSERDY) == RESET); // 等待晶振稳定

//        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSE); // 选择LSE作为RTC时钟源
//        RCC_RTCCLKCmd(ENABLE);        // 使能RTC时钟
//        
//        /* 等待RTC寄存器同步 */
//        RTC_WaitForSynchro();
//        RTC_WaitForLastTask();        // 确保寄存器可写
//        
//        /* 配置预分频器生成1Hz时钟：(32768 / (32768-1 + 1)) */
//        RTC_SetPrescaler(32768 - 1);  
//        RTC_WaitForLastTask();
//        
//        MyRTC_SetTime();              // 设置初始时间
//        
//        BKP_WriteBackupRegister(BKP_DR1, 0XA5A5); // 写入初始化标志
//    }
//    else 
//    {
//        /* 已初始化时的处理 */
//        RCC_RTCCLKCmd(ENABLE);   // 重新使能RTC时钟
//        RTC_WaitForSynchro();    // 同步寄存器
//    }
//}


/**
 * @brief  初始化实时时钟（RTC），强制使用内部 LSI 振荡器作为时钟源
 *
 * ──【设计背景与变更原因】──────────────────────────────────────
 * 原方案采用外部 LSE（32.768kHz 晶振）作为 RTC 时钟源，但在实际部署中发现：
 *   • LSE 极易因静电、焊接热损伤、电源噪声或过驱而永久失效；
 *   • 一旦 LSE 无法起振，系统将卡死在 RCC_FLAG_LSERDY 等待循环中，无法恢复；
 *   • 本设备为联网终端，时间由 SNTP 定期校准（如每小时一次），对本地 RTC 长期精度要求极低。
 *
 * 因此，自 2025-02-05 起，**彻底弃用 LSE，改用内部 LSI（～40kHz）作为 RTC 时钟源**。
 *
 * ──【优势】──────────────────────────────────────────────
 *   • 无需外部晶振及负载电容，BOM 更简洁，可靠性更高；
 *   • LSI 启动极快（<1ms），无死锁风险；
 *   • 系统鲁棒性显著提升，尤其适用于无 LSE 或 LSE 不稳定场景；
 *   • 时间误差（典型 ±5%～10%）可通过 SNTP 自动校正，不影响功能。
 *
 * ⚠️ 注意：LSI 频率标称约 40kHz（实际范围 30～50kHz），预分频器设为 39999，
 *         以获得近似 1Hz 的计时基准（即 (40000 / (39999 + 1)) ≈ 1Hz）。
 *
 * ──【初始化逻辑】──────────────────────────────────────────
 *   • 使用 BKP_DR1 寄存器存储初始化标志（0xA5A5）；
 *   • 首次上电（或断电后 BKP 清零）：启用 LSI → 配置 RTC → 设置默认时间；
 *   • 系统复位（NRST）或唤醒：检测到标志存在，仅恢复 RTC 时钟（需确保 LSI 已就绪！）；
 *   • 备份域需由 VBAT 或主电源维持，否则视为首次初始化。
 *
 * 🔥【关键修复：复位场景下的 LSI 状态问题】──────────────────────
 *   - 系统复位（NRST）不会清除备份域，BKP_DR1 仍为 0xA5A5；
 *   - 但 LSI 在复位后默认处于 **关闭状态**；
 *   - 若直接使能 RTC 时钟而不开启 LSI，RTC 将无输入时钟；
 *   - 导致 `RTC_WaitForSynchro()` 永久阻塞（死循环）。
 *
 * ✅ 修复措施：
 *   - 在“非首次初始化”分支中，**显式检查并启用 LSI**；
 *   - 确保 LSI 就绪后再调用 `RTC_WaitForSynchro()`。
 *
 * ──【测试建议】──────────────────────────────────────────
 *   1. 上电（无 VBAT） → 走首次初始化路径；
 *   2. 按 MCU 复位键 → 走恢复路径，应不卡死；
 *   3. 断电（有 VBAT）→ 走恢复路径，时间应保持。
 *
 * ──【调试支持】──────────────────────────────────────────
 *   如需查看本函数的详细初始化日志，请按以下步骤操作：
 *     1. 打开头文件 `debug_config.h`；
 *     2. 取消注释宏定义：`#define RTC_DEBUG_INIT`
 *        （例如：`// #define RTC_DEBUG_INIT` → `#define RTC_DEBUG_INIT`）
 *     3. 确保调试串口 `USART_DEBUG`（当前为 USART3）已在调用本函数前完成初始化；
 *   日志格式示例：[RTC][INIT] 初始化开始（使用LSI时钟源）...
 *
 * ──【未来若需恢复 LSE】────────────────────────────────────
 *   请务必同时满足以下条件：
 *     1. PCB 已焊接 32.768kHz 晶振及匹配负载电容（通常 2×10pF）；
 *     2. PC14/PC15 引脚配置为 `GPIO_Mode_IN_ANALOG`（禁用数字输入）；
 *     3. 所有 LSE 等待循环（如 `while(RCC_GetFlagStatus(RCC_FLAG_LSERDY)==RESET)`）
 *        必须增加超时保护，避免永久卡死。
 *
 * @warning 本实现 **不再依赖外部 32.768kHz 晶振**！
 */
void MyRTC_Init(void)
{
    RTC_LOG_INIT("初始化开始（使用LSI时钟源）...");

    /* 启用电源接口和备份域时钟 */
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_PWR, ENABLE);
    RCC_APB1PeriphClockCmd(RCC_APB1Periph_BKP, ENABLE);

    /* 允许访问备份寄存器 */
    PWR_BackupAccessCmd(ENABLE);

    /* 读取备份寄存器标志 */
    uint16_t bkreg = BKP_ReadBackupRegister(BKP_DR1);
    RTC_LOG_INIT("BKP_DR1 = 0x%04X", bkreg);

    /* 检查是否首次初始化 */
    if (bkreg != 0xA5A5) 
    {
        RTC_LOG_INIT("首次初始化：配置LSI...");
        
        /* 启用内部低速RC振荡器（LSI） */
        RCC_LSICmd(ENABLE);
        while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
        RTC_LOG_INIT("LSI 已就绪");

        /* 选择LSI作为RTC时钟源 */
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
        RCC_RTCCLKCmd(ENABLE);
        
        /* 等待RTC寄存器同步 */
        RTC_LOG_INIT("等待RTC同步...");
        RTC_WaitForSynchro();
        RTC_WaitForLastTask();
        RTC_LOG_INIT("RTC同步完成");
        
        /* 配置预分频器 */
        RTC_SetPrescaler(39999);  
        RTC_WaitForLastTask();
        RTC_LOG_INIT("预分频器设置完成（目标1Hz）");
        
        MyRTC_SetTime();
        RTC_LOG_INIT("初始时间已设置");
        
        BKP_WriteBackupRegister(BKP_DR1, 0xA5A5);
        RTC_LOG_INIT("备份寄存器标志已写入");
    }
    else 
    {
        RTC_LOG_INIT("已初始化：恢复RTC上下文...");

        // 🔥 关键修复：复位后 LSI 可能被关闭！必须重新启用并等待就绪
        if (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET) {
            RTC_LOG_INIT("LSI 未就绪，正在启用...");
            RCC_LSICmd(ENABLE);
            while (RCC_GetFlagStatus(RCC_FLAG_LSIRDY) == RESET);
            RTC_LOG_INIT("LSI 已就绪");
        } else {
            RTC_LOG_INIT("LSI 已处于就绪状态");
        }

        // 🔥 安全起见：显式重设时钟源（写入备份域寄存器，无副作用）
        RCC_RTCCLKConfig(RCC_RTCCLKSource_LSI);
        RCC_RTCCLKCmd(ENABLE);
        
        RTC_LOG_INIT("等待RTC同步...");
        RTC_WaitForSynchro();  // 现在 LSI 有信号，不会卡死
        RTC_LOG_INIT("RTC时钟已使能并同步");
    }

    RTC_LOG_INIT("初始化完成！");
}


/**
 * @brief  设置RTC计数器为指定日期时间的Unix时间戳
 * 
 * @note 本函数完成以下功能：
 * 1. 将用户自定义时间结构体转换为标准tm结构
 * 2. 进行时区修正（北京时间转UTC时间）
 * 3. 生成Unix时间戳并写入RTC计数器
 * 
 * @warning 硬件依赖：
 * - 依赖全局时间结构体Time存储原始时间参数
 * - 假设Time结构体包含Year/Month/Day/Hour/Minute/Second成员
 * - 需要RTC模块已正确初始化
 * 
 * @details 时间转换逻辑：
 * - tm_year从1900年开始计数，故需Time.Year-1900
 * - tm_month范围0-11，故需Time.Month-1
 * - 减去8*3600秒将北京时间(UTC+8)转换为UTC时间
 * 
 * 操作流程：
 * 1. 填充tm结构体：将用户时间转换为标准时间结构
 * 2. mktime生成时间戳：将本地时间转换为UTC秒数
 * 3. 时区修正：减去8小时（针对东八区时间输入）
 * 4. 写入RTC计数器：通过RTC_SetCounter设置当前时间
 * 5. 等待操作完成：确保寄存器写入成功
 */
void MyRTC_SetTime(void)
{
    time_t time_cnt;
    struct tm time_date;  
    
    /* 将用户自定义时间转换为标准tm结构 */
    time_date.tm_year = Time.Year - 1900;  // 年份从1900开始计数
    time_date.tm_mon = Time.Month - 1;    // 月份0-11对应1-12月
    time_date.tm_mday = Time.Day;
    time_date.tm_hour = Time.Hour;
    time_date.tm_min = Time.Minute;
    time_date.tm_sec = Time.Second;
	time_date.tm_wday = Time.wday;	
    
    /* 生成Unix时间戳并进行时区修正（假设输入时间为北京时间UTC+8） */
    time_cnt = mktime(&time_date) - 8*60*60;  // 转换为UTC时间戳
    
    /* 写入RTC计数器 */
    RTC_SetCounter(time_cnt);     // 设置RTC计数器值为UTC时间戳
    RTC_WaitForLastTask();        // 等待寄存器写入完成
}


/**
 * @brief 从RTC读取当前时间并转换为本地时间结构
 * @note 核心流程：
 * 1. 读取RTC计数器值(UTC时间戳)
 * 2. 增加8小时时区修正(适用于东八区)
 * 3. 转换为tm结构体
 * 4. 填充自定义Time结构
 */
void MyRTC_ReadTime(void)
{
    time_t time_cnt;
    struct tm time_date;
    
    /* 获取RTC计数器值并做时区修正 */
    time_cnt = RTC_GetCounter() + 8*60*60; // UTC+8转换为北京时间
    
    /* 将时间戳转换为本地时间结构 */
    time_date = *localtime(&time_cnt);     // 使用标准库时间转换函数
    
    /* 转换tm结构到自定义时间格式 */
    Time.Year = time_date.tm_year + 1900;  // tm_year从1900开始计数
    Time.Month = time_date.tm_mon + 1;     // tm_mon范围0-11对应1-12月
    Time.Day = time_date.tm_mday;
    Time.Hour = time_date.tm_hour;
    Time.Minute = time_date.tm_min;
    Time.Second = time_date.tm_sec;
	Time.wday = time_date.tm_wday;
}

