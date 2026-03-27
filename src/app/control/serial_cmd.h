/**
 * @file serial_cmd.h
 * @brief 串口命令解析与 PID 参数远程配置模块
 * @author Eureka
 * @date 2026-03-20
 * 
 * 功能特性:
 * - 健壮的接收状态机，支持容错处理
 * - 解析 `[angle,1.3,0,1.5]` 格式的命令包
 * - 原子性更新 PID 参数（双缓冲机制）
 * - 支持直立环 (angle)、速度环 (speed)、转向环 (turn)
 */

#ifndef SERIAL_CMD_H
#define SERIAL_CMD_H

#include <stdint.h>
#include <stdbool.h>
#include "PID.h"

// ============ 通信协议规范 ============

/**
 * 数据包格式：[<target>,Kp,Ki,Kd]
 * - 包头：'['
 * - 目标标识："angle" | "speed" | "turn"
 * - 参数：3 个浮点数（逗号分隔）
 * - 包尾：']'
 * 
 * 示例：
 * - [angle,1.3,0,1.5]  → 设置直立环 Kp=1.3, Ki=0, Kd=1.5
 * - [speed,2.5,0.1,0.2] → 设置速度环 Kp=2.5, Ki=0.1, Kd=0.2
 * - [turn,3.0,0,0.5]    → 设置转向环 Kp=3.0, Ki=0, Kd=0.5
 */

// ============ 接收状态机定义 ============

typedef enum {
    WAIT_START_BRACKET,   // 等待包头 '['
    PARSE_TARGET,         // 解析目标标识
    PARSE_KP,             // 解析 Kp
    PARSE_KI,             // 解析 Ki
    PARSE_KD,             // 解析 Kd
    WAIT_END_BRACKET,     // 等待包尾 ']'
    PARSE_ERROR           // 解析错误（容错状态）
} SerialCmdState_t;

// ============ 命令解析器句柄 ============

typedef struct {
    SerialCmdState_t state;      // 当前状态
    char target_buf[16];         // 目标标识缓冲区
    uint8_t target_idx;          // 目标缓冲区索引
    char param_buf[3][32];       // 3 个参数缓冲区
    uint8_t param_idx;           // 当前参数索引
    uint8_t char_count[3];       // 每个参数的字符计数
    bool is_ready;               // 命令解析完成标志
    
    // 统计信息（用于调试）
    uint32_t total_commands;     // 成功解析的命令总数
    uint32_t error_count;        // 解析错误次数
} SerialCmdParser_t;

// ============ 全局接口 ============

/**
 * @brief 初始化串口命令解析器
 */
void SerialCmd_Init(void);

/**
 * @brief 处理串口接收数据（在主循环中调用）
 * @param data 接收到的字节
 * 
 * @note 此函数会被多次调用处理每个接收字节
 *       当解析完整个命令包后，is_ready 标志置位
 */
void SerialCmd_ProcessByte(uint8_t data);

/**
 * @brief 检查是否有待处理的命令
 * @return true: 有命令待处理 | false: 无命令
 */
bool SerialCmd_IsReady(void);

/**
 * @brief 应用已解析的 PID 参数（原子操作）
 * 
 * @note 应在主循环安全位置调用（不在中断中）
 */
void SerialCmd_ApplyPendingParams(void);

/**
 * @brief 获取解析器句柄（用于调试）
 * @return 解析器状态指针
 */
SerialCmdParser_t* SerialCmd_GetParser(void);

// ============ 遥测数据上传接口 ============

/**
 * @brief 发送完整遥测数据（CSV 格式）
 * 
 * @param timestamp 时间戳（ms）
 * 
 * @note 输出格式：
 * Time,Angle_Target,Angle_Actual,Angle_Out,Speed_Target,Speed_Actual,Speed_Out,Turn_Target,Turn_Actual,Turn_Out,Kp_Angle,Ki_Angle,Kd_Angle
 */
void Telemetry_SendFullData(uint32_t timestamp);

/**
 * @brief 发送简化遥测数据（仅关键变量）
 * 
 * @param timestamp 时间戳（ms）
 * 
 * @note 输出格式：
 * Time,Angle,PWM,Kp,Ki,Kd
 */
void Telemetry_SendSimpleData(uint32_t timestamp);

#endif // SERIAL_CMD_H
