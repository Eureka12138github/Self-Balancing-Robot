#ifndef CONTROL_H_
#define CONTROL_H_

#include <stdint.h>
#include <stdbool.h>

// ============ 调试模式配置 ============
typedef enum {
    DEBUG_MODE_OFF = 0,      // 关闭调试输出
    DEBUG_MODE_BENCH = 1,    // 台架测试（高频数据）
    DEBUG_MODE_WIRED = 2,    // 有线调试（CSV 格式）
    DEBUG_MODE_WIRELESS = 3  // 无线调试（精简格式）
} DebugMode_t;

// 在此处修改调试模式
#define CURRENT_DEBUG_MODE DEBUG_MODE_WIRED

// ============ 数据结构 ============
typedef struct {
    float angle;           // 融合角度
    float angleAcc;        // 加速度计角度
    float angleGyro;       // 陀螺仪积分角度
    float gyro;            // 角速度
    float pwm_out;         // 直立环 PWM 输出
    float speed;           // 车速
    float target_angle;    // 目标角度
    bool run_flag;         // 运行标志
} BalanceData_t;

// 全局数据接口（供调试读取）
extern BalanceData_t balance_data;

void Balance_Control_Loop(void);
void Balance_Init(void);
void Balance_SetDebugMode(DebugMode_t mode);
void Balance_SendTelemetry(void);  // 发送遥测数据

// ============ 新增：外部控制接口 ============
void Control_ProcessCommands(void);  // 处理串口命令

#endif
