#include "storage.h"
#include "PID.h"      // 添加 PID_t 类型支持
#include <string.h>   // 提供 memcpy 函数

/**
 * @brief Flash 存储区起始地址（位于主 Flash 最后一页）
 * 
 * 该地址必须对齐到 Flash 页边界（如 STM32F103 的 1KB 页，0x800FC00 是典型末页地址）。
 * 整个页用于存储持久化数据，共 1024 字节（512 个 u16 单元）。
 */
#define STORE_START_ADDRESS 0x800FC00

/**
 * @brief 存储缓冲区大小（单位：u16，即半字）
 * 
 * 对应 Flash 一页 1024 字节（512 × 2 字节），其中：
 * - Store_Data[0]：保留为初始化标志位（值为 0xA5A5 表示已初始化）
 * - Store_Data[1..STORE_COUNT-1]：用户数据区
 */
#define STORE_COUNT 512

/**
 * @brief 内存中的持久化数据缓存数组
 * 
 * - 大小：512 × u16 = 1024 字节，与 Flash 一页对齐。
 * - Store_Data[0] 是特殊标志位，**禁止在应用层直接修改**！
 *   应通过 Store_Init() / Store_Save() 管理。
 * 
 * @warning 修改 Store_Data[0] 可能导致系统误判为"未初始化"，从而擦除所有历史数据！
 */
u16 Store_Data[STORE_COUNT];

/*===========================================================================
 *                      PID 参数存储辅助函数实现
 *===========================================================================*/

/**
 * @brief 保存单个 float 参数到 Store_Data（拆分为 2 个 u16）
 * 
 * @param value 要保存的 float 值
 * @param idx Store_Data 中的起始索引（占用 2 个单元）
 */
static void Store_Float(float value, uint16_t idx)
{
    union {
        float f;
        uint32_t u32;
        uint16_t u16[2];
    } converter;
    
    converter.f = value;
    
    // 小端模式存储：低 16 位在前，高 16 位在后
    Store_Data[idx] = converter.u16[0];
    Store_Data[idx + 1] = converter.u16[1];
}

/**
 * @brief 从 Store_Data 加载单个 float 参数（重组 2 个 u16）
 * 
 * @param idx Store_Data 中的起始索引
 * @return 重组后的 float 值
 */
static float Load_Float(uint16_t idx)
{
    union {
        float f;
        uint32_t u32;
        uint16_t u16[2];
    } converter;
    
    // 小端模式读取
    converter.u16[0] = Store_Data[idx];
    converter.u16[1] = Store_Data[idx + 1];
    
    return converter.f;
}

/**
 * @brief 保存 PID 参数到 Flash
 */
void PID_Store_Save(const void* pid, uint16_t base_idx)
{
    if (pid == NULL) return;
    
    const PID_t* pid_ptr = (const PID_t*)pid;
    
    // 保存 Kp, Ki, Kd（每个参数拆分为 2 个 u16）
    Store_Float(pid_ptr->Kp, base_idx);
    Store_Float(pid_ptr->Ki, base_idx + 2);
    Store_Float(pid_ptr->Kd, base_idx + 4);
    
    // 保存到 Flash（调用 Store_Save 会写入整个页）
    // ⚠️ 注意：这里不直接调用 Store_Save，由上层决定何时批量保存
}

/**
 * @brief 从 Flash 加载 PID 参数
 */
bool PID_Store_Load(void* pid, uint16_t base_idx)
{
    if (pid == NULL) return false;
    
    PID_t* pid_ptr = (PID_t*)pid;
    
    // 检查数据是否有效（全 0 或 NaN 视为无效）
    float kp = Load_Float(base_idx);
    float ki = Load_Float(base_idx + 2);
    float kd = Load_Float(base_idx + 4);
    
    // 有效性检查
    if ((kp != kp) || (ki != ki) || (kd != kd)) {  // NaN 检查
        return false;
    }
    
    // 范围检查（防止异常大的值）
    if (kp < -100.0f || kp > 100.0f ||
        ki < -100.0f || ki > 100.0f ||
        kd < -100.0f || kd > 100.0f) {
        return false;
    }
    
    // 加载参数
    pid_ptr->Kp = kp;
    pid_ptr->Ki = ki;
    pid_ptr->Kd = kd;
    
    return true;
}

/**
 * @brief 清除 Flash 中存储的 PID 参数（恢复出厂设置）
 * 
 * @param base_idx PID 参数在 Store_Data 中的起始索引
 * 
 * @note 将指定索引的 6 个 u16 单元清零（Kp/Ki/Kd 三个 float）
 *       不会立即保存到 Flash，需上层调用 Store_Save()
 */
void PID_Store_Clear(uint16_t base_idx)
{
    // 清零 Kp, Ki, Kd（每个参数占用 2 个 u16）
    Store_Data[base_idx] = 0x0000;
    Store_Data[base_idx + 1] = 0x0000;
    Store_Data[base_idx + 2] = 0x0000;
    Store_Data[base_idx + 3] = 0x0000;
    Store_Data[base_idx + 4] = 0x0000;
    Store_Data[base_idx + 5] = 0x0000;
}

/**
 * @brief 初始化持久化存储模块
 * 
 * 功能：
 * - 检查 Flash 存储区是否已初始化（通过 Store_Data[0] == 0xA5A5 判断）。
 * - 若未初始化：
 *   1. 擦除整个存储页；
 *   2. 写入标志位 0xA5A5 到首地址；
 *   3. 将其余 511 个单元初始化为 0。
 * - 无论是否初始化，最终将整页 Flash 数据加载到 Store_Data[] 数组。
 * 
 * @note 必须在系统启动早期调用（如 main() 开头），确保后续操作基于有效数据。
 * @note 初始化后，Store_Data[0] = 0xA5A5，用户数据从索引 1 开始使用。
 */
void Store_Init(void)
{
    if (MyFLASH_ReadHalfWord(STORE_START_ADDRESS) != 0xA5A5) {
        // 首次使用：擦除并初始化整页
        MyFLASH_ErasePage(STORE_START_ADDRESS);
        MyFLASH_ProgramHalfWord(STORE_START_ADDRESS, 0xA5A5);
        
        // 初始化用户数据区（索引 1 ～ STORE_COUNT-1）
        for (u16 i = 1; i < STORE_COUNT; i++) {
            MyFLASH_ProgramHalfWord(STORE_START_ADDRESS + i * 2, 0x0000);
        }
    }

    // 从 Flash 加载整页数据到内存缓冲区
    for (u16 i = 0; i < STORE_COUNT; i++) {
        Store_Data[i] = MyFLASH_ReadHalfWord(STORE_START_ADDRESS + i * 2);
    }
}

/**
 * @brief 将内存中的 Store_Data[] 同步保存到 Flash
 * 
 * 功能：
 * - 擦除当前存储页；
 * - 将整个 Store_Data 数组（含标志位）重新写入 Flash。
 * 
 * @warning 此操作会触发一次完整页擦除 + 512 次半字编程，
 *          频繁调用会加速 Flash 磨损，应限制调用频率。
 * @note 调用前确保 Store_Data[0] 仍为 0xA5A5（不要被意外覆盖）。
 */
void Store_Save(void)
{
    MyFLASH_ErasePage(STORE_START_ADDRESS);
    for (u16 i = 0; i < STORE_COUNT; i++) {
        MyFLASH_ProgramHalfWord(STORE_START_ADDRESS + i * 2, Store_Data[i]);
    }
}

/**
 * @brief 清除用户数据区（保留初始化标志位）
 * 
 * 功能：
 * - 将 Store_Data[1] 至 Store_Data[STORE_COUNT-1] 清零；
 * - 调用 Store_Save() 持久化到 Flash。
 * 
 * @note Store_Data[0]（标志位）不会被清除，确保系统仍识别为"已初始化"。
 * @note 通常由用户通过 UI 触发（如"清除错误日志"）。
 */
void Store_Clear(void)
{
    for (u16 i = 1; i < STORE_COUNT; i++) {
        Store_Data[i] = 0x0000;
    }
    Store_Save();
}
