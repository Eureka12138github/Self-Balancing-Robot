#ifndef STORAGE_H
#define STORAGE_H

#include "stm32f10x.h"
#include "flash.h"

/**
 * @brief 持久化存储缓冲区（定义在 storage.c 中）
 * 
 * - 总大小：512 × u16 = 1024 字节（对应 Flash 一页）
 * - Store_Data[0]：保留为初始化标志位（0xA5A5），禁止应用层修改！
 * - 用户数据从索引 1 开始分配。
 */
extern u16 Store_Data[];

/*===========================================================================
 *                          存储区域划分说明
 *===========================================================================
 *
 * 整个 Store_Data 数组按功能划分为多个区域，各区域起始索引如下：
 *
 * | 索引范围       | 用途                     | 说明
 * |----------------|--------------------------|-------------------------------
 * | [0]            | 初始化标志位             | 固定为 0xA5A5，不可用于用户数据
 * | [1] ～ [9]      | 系统配置参数             | 如阈值、计数器等
 * | [10] ～ [36]    | 错误日志（ErrorTime[3]） | 每条日志占 9 个 u16
 * | [40] ～ [66]    | 警报日志（预留）         | 暂未启用，保留以防未来扩展
 * | [67] ～ [511]   | 未使用                   | 可供后续功能扩展
 *
 * ⚠️ 重要约束：
 *   - 所有用户数据索引必须 ≥ 1；
 *   - 不同功能区域不得重叠；
 *   - 修改布局前需同步更新 storage.c 中的序列化逻辑。
 */

/*===========================================================================
 *                      系统配置参数存储索引（1 ～ 9）
 *===========================================================================*/
#define DUST_LIMIT_STORE_IDX        (1U)    ///< 扬尘阈值（u16）
#define NOISE_LIMIT_STORE_IDX       (2U)    ///< 噪音阈值（u16）
#define RESET_TIMERS_STORE_IDX      (3U)    ///< 允许的最大复位次数阈值（u16）
#define ERROR_LOG_STORE_IDX         (4U)    ///< 错误日志写入指针（u16，取值 0～2）

// 可在此处继续添加其他系统参数（确保 ≤ 9）

/*===========================================================================
 *                      错误日志存储布局（索引 10 起）
 *===========================================================================*/
#define ERROR_LOG_ENTRY_SIZE        (9U)    ///< 每条错误日志占用的 u16 单元数
#define ERROR_TIME_ARRAY_SIZE       (3U)    ///< 最多支持 3 条错误记录

// 错误日志各字段在单条记录中的偏移（相对于条目起始）
#define ERR_LOG_YEAR_OFFSET         (0U)
#define ERR_LOG_MONTH_OFFSET        (1U)
#define ERR_LOG_DAY_OFFSET          (2U)
#define ERR_LOG_HOUR_OFFSET         (3U)
#define ERR_LOG_MINUTE_OFFSET       (4U)
#define ERR_LOG_SECOND_OFFSET       (5U)
#define ERR_LOG_WDAY_OFFSET         (6U)
#define ERR_LOG_TYPE_OFFSET         (7U)
#define ERR_LOG_SHOW_FLAG_OFFSET    (8U)

/**
 * @brief 获取第 n 条错误日志在 Store_Data 中的起始索引
 * @param n 日志序号（0, 1, 2）
 * @return 对应的 Store_Data 起始索引
 */
#define ERROR_LOG_START_INDEX(n)    (10U + (n) * ERROR_LOG_ENTRY_SIZE)

// 静态断言：确保日志区域不越界（编译期检查）
#if (ERROR_LOG_START_INDEX(ERROR_TIME_ARRAY_SIZE - 1) + ERROR_LOG_ENTRY_SIZE) > 40
#error "Error log region exceeds reserved space! Check layout in storage.h"
#endif

/*===========================================================================
 *                      警报日志区域（预留，暂未使用）
 *===========================================================================*/
// #define ALARM_LOG_START_INDEX     (40U)  // 预留：40 ～ 66（共 27 个 u16，可存 3 条警报）
// 注意：当前警报信息不写入 Flash，此区域仅作预留

/*===========================================================================
 *                          函数声明
 *===========================================================================*/

/**
 * @brief 初始化持久化存储模块
 * 
 * - 检查 Flash 是否已初始化（标志位 0xA5A5）；
 * - 若未初始化，则擦除并清零用户数据区；
 * - 将 Flash 数据加载到 Store_Data[]。
 * 
 * @note 必须在 main() 早期调用。
 */
void Store_Init(void);

/**
 * @brief 将 Store_Data[] 同步保存到 Flash
 * 
 * - 擦除整页后重新写入全部 512 个 u16；
 * - 包含标志位和所有用户数据。
 * 
 * @warning 频繁调用会加速 Flash 磨损。
 */
void Store_Save(void);

/**
 * @brief 清除用户数据（保留初始化标志位）
 * 
 * - 清零 Store_Data[1] 至 Store_Data[511]；
 * - 调用 Store_Save() 持久化。
 * 
 * @note 通常由 UI 触发（如“清除日志”）。
 */
void Store_Clear(void);

#endif /* STORAGE_H */

