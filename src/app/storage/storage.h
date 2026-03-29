#ifndef STORAGE_H
#define STORAGE_H

#include "stm32f10x.h"
#include "flash.h"
#include <stdbool.h>  // 添加 bool 类型支持

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
 * | [67] ～ [93]    | PID 参数存储区           | 9 个 PID 参数×3 个 u16=27 单元
 * | [94] ～ [511]   | 未使用                   | 可供后续功能扩展
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
 *                      PID 参数存储区（索引 67 ～ 93）
 *===========================================================================
 * 
 * 存储 9 个 PID 参数（直立环、速度环、转向环各 3 个参数）
 * 每个 float 参数拆分为 2 个 u16 存储（IEEE754 单精度浮点数，4 字节）
 * 
 * 布局说明：
 * - 每个 PID 参数占用 2 个连续的 u16
 * - 第 1 个 u16：低 16 位
 * - 第 2 个 u16：高 16 位（包含符号位和指数）
 */

#define PID_STORAGE_BASE_INDEX    (67U)   ///< PID 参数存储起始索引
#define PID_PARAM_SIZE            (2U)    ///< 每个 PID 参数占用 2 个 u16（float=4 字节）
#define PID_GROUP_SIZE            (6U)    ///< 每组 PID（Kp+Ki+Kd）占用 6 个 u16

// 直立环 PID 参数索引（Angle PID）
#define PID_ANGLE_KP_IDX          (PID_STORAGE_BASE_INDEX + 0)
#define PID_ANGLE_KI_IDX          (PID_STORAGE_BASE_INDEX + 2)
#define PID_ANGLE_KD_IDX          (PID_STORAGE_BASE_INDEX + 4)

// 速度环 PID 参数索引（Speed PID）
#define PID_SPEED_KP_IDX          (PID_STORAGE_BASE_INDEX + 6)
#define PID_SPEED_KI_IDX          (PID_STORAGE_BASE_INDEX + 8)
#define PID_SPEED_KD_IDX          (PID_STORAGE_BASE_INDEX + 10)

// 转向环 PID 参数索引（Turn PID）
#define PID_TURN_KP_IDX           (PID_STORAGE_BASE_INDEX + 12)
#define PID_TURN_KI_IDX           (PID_STORAGE_BASE_INDEX + 14)
#define PID_TURN_KD_IDX           (PID_STORAGE_BASE_INDEX + 16)

// 静态断言：确保 PID 存储区域不越界（9 个参数×2 + 67 = 85 < 94）
#if (PID_STORAGE_BASE_INDEX + PID_GROUP_SIZE * 3) > 94
#error "PID storage region exceeds reserved space! Check layout in storage.h"
#endif

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

/*===========================================================================
 *                      PID 参数存储辅助函数
 *===========================================================================*/

/**
 * @brief 保存 PID 参数到 Flash
 * 
 * @param pid 指向 PID_t 结构体的指针
 * @param base_idx PID 参数在 Store_Data 中的起始索引
 * 
 * @note 将 Kp/Ki/Kd 三个 float 参数拆分为 6 个 u16 存储
 */
void PID_Store_Save(const void* pid, uint16_t base_idx);

/**
 * @brief 从 Flash 加载 PID 参数
 * 
 * @param pid 指向 PID_t 结构体的指针（输出）
 * @param base_idx PID 参数在 Store_Data 中的起始索引
 * 
 * @return true: 加载成功 | false: 数据无效（返回默认值 0）
 * 
 * @note 将 6 个 u16 重组为 Kp/Ki/Kd 三个 float 参数
 */
bool PID_Store_Load(void* pid, uint16_t base_idx);

/**
 * @brief 清除 Flash 中存储的 PID 参数（恢复出厂设置）
 * 
 * @param base_idx PID 参数在 Store_Data 中的起始索引
 * 
 * @note 将指定索引的 6 个 u16 单元清零（Kp/Ki/Kd 三个 float）
 *       不会立即保存到 Flash，需上层调用 Store_Save()
 */
void PID_Store_Clear(uint16_t base_idx);

#endif /* STORAGE_H */

