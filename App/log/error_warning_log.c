#include "error_warning_log.h"
/**
 * @brief 将指定索引的错误时间信息存储到 Store_Data 数组中
 * @param index 错误日志索引（0 ～ ERROR_TIME_ARRAY_SIZE-1）
 */
void StoreErrorTime(u8 index) {
    // 使用 storage.h 中定义的宏计算起始位置
    u16 baseIndex = ERROR_LOG_START_INDEX(index);

    Store_Data[baseIndex + ERR_LOG_YEAR_OFFSET]      = ErrorTime[index].errortime.Year;
    Store_Data[baseIndex + ERR_LOG_MONTH_OFFSET]     = ErrorTime[index].errortime.Month;
    Store_Data[baseIndex + ERR_LOG_DAY_OFFSET]       = ErrorTime[index].errortime.Day;
    Store_Data[baseIndex + ERR_LOG_HOUR_OFFSET]      = ErrorTime[index].errortime.Hour;
    Store_Data[baseIndex + ERR_LOG_MINUTE_OFFSET]    = ErrorTime[index].errortime.Minute;
    Store_Data[baseIndex + ERR_LOG_SECOND_OFFSET]    = ErrorTime[index].errortime.Second;
    Store_Data[baseIndex + ERR_LOG_WDAY_OFFSET]      = ErrorTime[index].errortime.wday;
    Store_Data[baseIndex + ERR_LOG_TYPE_OFFSET]      = ErrorTime[index].errortype;
    Store_Data[baseIndex + ERR_LOG_SHOW_FLAG_OFFSET] = ErrorTime[index].errorshowflag;
}

/**
 * @brief 将指定索引的警报时间信息存储到 Store_Data 数组中
 * @param index 警报日志索引（0 ～ WARNING_TIME_ARRAY_SIZE-1）
 * @note 当前警报日志功能暂未启用（见 storage.h 预留区说明）
 */
void StoreWarningTime(u8 index) {
    // 若未来启用，此处应类似错误日志实现
    // 目前保留空实现或断言禁用
    (void)index; // 避免编译警告
}

/**
 * @brief 从 Store_Data 加载所有有效警报记录到 WarningTime 数组
 * @note 当前警报日志未启用，此函数可保留为空或用于调试
 */
void ReadStoreWarningTime(void) {
    // 暂不实现，因警报日志未写入 Flash
}

/**
 * @brief 从 Store_Data 加载所有有效错误记录到 ErrorTime 数组
 */
void ReadStoreErrorTime(void) {
    for (u8 i = 0; i < ERROR_TIME_ARRAY_SIZE; i++) {
        u16 baseIndex = ERROR_LOG_START_INDEX(i);
        
        // 仅加载已标记为有效的记录
        if (Store_Data[baseIndex + ERR_LOG_SHOW_FLAG_OFFSET] != 0) {
            ErrorTime[i].errortime.Year   = Store_Data[baseIndex + ERR_LOG_YEAR_OFFSET];
            ErrorTime[i].errortime.Month  = Store_Data[baseIndex + ERR_LOG_MONTH_OFFSET];
            ErrorTime[i].errortime.Day    = Store_Data[baseIndex + ERR_LOG_DAY_OFFSET];
            ErrorTime[i].errortime.Hour   = Store_Data[baseIndex + ERR_LOG_HOUR_OFFSET];
            ErrorTime[i].errortime.Minute = Store_Data[baseIndex + ERR_LOG_MINUTE_OFFSET];
            ErrorTime[i].errortime.Second = Store_Data[baseIndex + ERR_LOG_SECOND_OFFSET];
            ErrorTime[i].errortime.wday   = Store_Data[baseIndex + ERR_LOG_WDAY_OFFSET];
            ErrorTime[i].errortype        = Store_Data[baseIndex + ERR_LOG_TYPE_OFFSET];
            ErrorTime[i].errorshowflag    = Store_Data[baseIndex + ERR_LOG_SHOW_FLAG_OFFSET];
        } else {
            // 可选：清空无效条目（确保 RAM 状态一致）
            memset(&ErrorTime[i], 0, sizeof(ErrorTime[i]));
        }
    }
}

/**
 * @brief 记录错误类型及发生时间，并持久化存储到 Flash
 * （注释同前，此处省略以节省篇幅）
 */
void ErrorType(u8 Type) {
    if (Type == 0) return;

    u8 LogIndex = Store_Data[ERROR_LOG_STORE_IDX];

    if (LogIndex >= ERROR_TIME_ARRAY_SIZE) {
        LogIndex = 0;
        Store_Data[ERROR_LOG_STORE_IDX] = 0;
    }

    ErrorTime[LogIndex].errortype = Type;
    ErrorTime[LogIndex].errorshowflag = 1; // true -> 1（u16 存储）

    MyRTC_ReadTime();
    ErrorTime[LogIndex].errortime.Year   = Time.Year;
    ErrorTime[LogIndex].errortime.Month  = Time.Month;
    ErrorTime[LogIndex].errortime.Day    = Time.Day;
    ErrorTime[LogIndex].errortime.Hour   = Time.Hour;
    ErrorTime[LogIndex].errortime.Minute = Time.Minute;
    ErrorTime[LogIndex].errortime.Second = Time.Second;
    ErrorTime[LogIndex].errortime.wday   = Time.wday;

    StoreErrorTime(LogIndex);

    // 更新并持久化写入指针
	if(++LogIndex == ERROR_TIME_ARRAY_SIZE){
		LogIndex = 0;
	}	
    Store_Data[ERROR_LOG_STORE_IDX] = LogIndex;

    Store_Save();//调试阶段暂不开启存储功能
}

/**
 * @brief 记录警报类型（当前仅存于 RAM，不写入 Flash）
 *
 * - 因警报信息频繁触发，为避免 Flash 过度磨损，暂不持久化；
 * - 使用静态索引实现环形缓冲，仅在 RAM 中保留最近几条。
 *
 * @param Type 警报类型（非 0 有效）
 */
void WarningType(u8 Type) {
    if (Type == 0) return;

    static u8 LogIndex = 0;

    WarningTime[LogIndex].warningtype = Type;
    WarningTime[LogIndex].warningshowflag = 1;

    MyRTC_ReadTime();
    WarningTime[LogIndex].warningtime.Year   = Time.Year;
    WarningTime[LogIndex].warningtime.Month  = Time.Month;
    WarningTime[LogIndex].warningtime.Day    = Time.Day;
    WarningTime[LogIndex].warningtime.Hour   = Time.Hour;
    WarningTime[LogIndex].warningtime.Minute = Time.Minute;
    WarningTime[LogIndex].warningtime.Second = Time.Second;
    WarningTime[LogIndex].warningtime.wday   = Time.wday;

    // 环形递增（仅 RAM，无 Flash 同步）
	if(++LogIndex == WARNING_TIME_ARRAY_SIZE){
		LogIndex = 0;
	}
}

