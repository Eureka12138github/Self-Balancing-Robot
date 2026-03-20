/**
 * @file serial_cmd.c
 * @brief 串口命令解析与 PID 参数远程配置实现
 * @author Eureka
 * @date 2026-03-20
 */

#include "serial_cmd.h"
#include "usart.h"       // 使用 USART1 发送响应
#include "system_config.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

// ============ 全局变量声明（来自 system_config.c） ============
extern PID_t anglePID;
extern PID_t speedPID;
extern PID_t turnPID;

// ============ 静态变量 ============

static SerialCmdParser_t g_parser = {0};
static volatile bool g_initialized = false;  // 初始化标志

// 外部 PID 实例指针数组（用于查找匹配）
static struct {
    const char* name;
    PID_t* pid;
} g_pid_map[] = {
    {"angle", &anglePID},
    {"speed", &speedPID},
    {"turn",  &turnPID}
};

#define PID_MAP_SIZE (sizeof(g_pid_map) / sizeof(g_pid_map[0]))

// ============ 内部辅助函数 ============

/**
 * @brief 重置解析器状态（容错处理）
 */
static void reset_parser(void)
{
    g_parser.state = WAIT_START_BRACKET;
    g_parser.target_idx = 0;
    g_parser.param_idx = 0;
    for (int i = 0; i < 3; i++) {
        g_parser.char_count[i] = 0;
        g_parser.param_buf[i][0] = '\0';
    }
    g_parser.is_ready = false;
}

/**
 * @brief 去除字符串两端空白字符
 */
static char* trim(char* str)
{
    if (str == NULL) return NULL;
    
    // 去除前导空白
    while (isspace((unsigned char)*str)) str++;
    
    if (*str == '\0') return str;
    
    // 去除尾部空白
    char* end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) end--;
    *(end + 1) = '\0';
    
    return str;
}

/**
 * @brief 根据目标标识查找对应的 PID 实例
 */
static PID_t* find_pid_by_name(const char* name)
{
    for (size_t i = 0; i < PID_MAP_SIZE; i++) {
        if (strcmp(g_pid_map[i].name, name) == 0) {
            return g_pid_map[i].pid;
        }
    }
    return NULL;
}

/**
 * @brief 发送错误响应
 * 注意：直接使用 Serial_Printf，依赖环形缓冲区
 */
#define send_error_response(msg)  Serial_Printf(USART1, "%s\r\n", msg)

/**
 * @brief 应用解析后的 PID 参数
 */
static void apply_parsed_params(void)
{
    // 查找目标 PID
    char target_name[16];
    strncpy(target_name, g_parser.target_buf, sizeof(target_name) - 1);
    target_name[sizeof(target_name) - 1] = '\0';
    
    PID_t* pid = find_pid_by_name(trim(target_name));
    if (pid == NULL) {
        g_parser.error_count++;
        reset_parser();
        
        // ❌ 错误响应：未找到目标 PID
        char buf[40];
        snprintf(buf, sizeof(buf), "[ERROR]Unknown target: %s", target_name);
        send_error_response(buf);
        return;
    }
    
    // 解析 3 个浮点参数并验证
    float params[3];
    for (int i = 0; i < 3; i++) {
        char* param_str = trim(g_parser.param_buf[i]);
        
        // ✅ 检查空参数
        if (param_str[0] == '\0') {
            g_parser.error_count++;
            reset_parser();
            Serial_Printf(USART1, "[ERROR]Empty parameter %d\r\n", i + 1);
            return;
        }
        
        // ✅ 解析并检查有效性
        char* endptr = NULL;  // 初始化为 NULL
        params[i] = strtof(param_str, &endptr);
        
        // 防御性检查：endptr 应该被设置
        if (endptr == NULL) {
            g_parser.error_count++;
            reset_parser();
            Serial_Printf(USART1, "[ERROR]strtof failed for parameter %d\r\n", i + 1);
            return;
        }
        
        // 检查是否完全解析（endptr 应该指向\0）
        if (*endptr != '\0') {
            g_parser.error_count++;
            reset_parser();
            Serial_Printf(USART1, "[ERROR]Invalid parameter %d: %s\r\n", i + 1, param_str);
            return;
        }
        
        // ✅ 检查 NaN 和异常大值
        if (params[i] != params[i] ||  // NaN 检查
            params[i] > 1000.0f || params[i] < -1000.0f) {  // PID 参数不可能超过 1000
            g_parser.error_count++;
            reset_parser();
            Serial_Printf(USART1, "[ERROR]Parameter %d invalid: %s\r\n", i + 1, param_str);
            return;
        }
    }
    
    // 参数范围检查（安全保护）
    if (params[0] < 0 || params[0] > 100 ||  // Kp 范围检查
        params[1] < 0 || params[1] > 100 ||  // Ki 范围检查
        params[2] < 0 || params[2] > 100) {  // Kd 范围检查
        g_parser.error_count++;
        reset_parser();
        
        // ❌ 错误响应：参数超出范围
        char buf[60];
        snprintf(buf, sizeof(buf), "[ERROR]Params out of range: %.3f,%.3f,%.3f", 
                 params[0], params[1], params[2]);
        send_error_response(buf);
        return;
    }
    
    // 使用双缓冲机制安全更新参数
    PID_SetParams(pid, params[0], params[1], params[2]);
    
    g_parser.total_commands++;
    reset_parser();
    
    // ✅ 成功响应
    Serial_Printf(USART1, "[OK]%s:%.3f,%.3f,%.3f\r\n", 
                 target_name, params[0], params[1], params[2]);
}

// ============ 公开 API 实现 ============

/**
 * @brief 初始化串口命令解析器
 */
void SerialCmd_Init(void)
{
    reset_parser();
    g_parser.total_commands = 0;
    g_parser.error_count = 0;
    g_initialized = true;  // 标记已初始化
    
    // 发送就绪提示
    Serial_Printf(USART1, "[READY]PID tuner ready. Send: [angle,Kp,Ki,Kd]\r\n");
}

/**
 * @brief 处理串口接收字节（状态机核心）
 */
void SerialCmd_ProcessByte(uint8_t data)
{
    // 如果还未初始化，直接忽略
    if (!g_initialized) {
        return;
    }
    
    switch (g_parser.state)
    {
        case WAIT_START_BRACKET:
            if (data == '[') {
                g_parser.state = PARSE_TARGET;
                g_parser.target_idx = 0;
                g_parser.target_buf[0] = '\0';
            } else if (data == '\n' || data == '\r') {
                // 忽略换行符（串口助手可能自动添加）
                // 保持在 WAIT_START_BRACKET 状态
            }
            // 其他字符忽略（容错）
            break;
            
        case PARSE_TARGET:
            if (data == ',') {
                g_parser.target_buf[g_parser.target_idx] = '\0';
                g_parser.state = PARSE_KP;
                g_parser.param_idx = 0;
                g_parser.char_count[0] = 0;
            } else if (g_parser.target_idx < sizeof(g_parser.target_buf) - 1) {
                g_parser.target_buf[g_parser.target_idx++] = data;
            } else {
                // 缓冲区溢出，进入错误状态
                g_parser.error_count++;
                
                // ❌ 错误响应：目标标识过长
                send_error_response("[ERROR]Target too long (max 15 chars)");
                
                // 立即重置，等待下一个命令
                reset_parser();
            }
            break;
            
        case PARSE_KP:
        case PARSE_KI:
        case PARSE_KD:
            if (data == ',') {
                // 切换到下一个参数
                if (g_parser.param_idx < 2) {
                    g_parser.param_buf[g_parser.param_idx][g_parser.char_count[g_parser.param_idx]] = '\0';
                    g_parser.param_idx++;
                    g_parser.char_count[g_parser.param_idx] = 0;
                    
                    // 更新状态
                    if (g_parser.param_idx == 1) {
                        g_parser.state = PARSE_KI;
                    } else if (g_parser.param_idx == 2) {
                        g_parser.state = PARSE_KD;
                    }
                } else {
                    // 参数过多，错误
                    g_parser.error_count++;
                    
                    // ❌ 错误响应：参数过多
                    send_error_response("[ERROR]Too many parameters (expected 3)");
                    
                    // 立即重置
                    reset_parser();
                }
            } else if (data == ']') {
                // 最后一个参数结束，立即应用解析结果
                g_parser.param_buf[g_parser.param_idx][g_parser.char_count[g_parser.param_idx]] = '\0';
                
                // 🔧 修复：检查当前状态
                if (g_parser.state == PARSE_KD) {
                    // ✅ 正常：已经解析完 3 个参数
                    apply_parsed_params();
                } else {
                    // ❌ 错误：在 PARSE_KP 或 PARSE_KI 就收到 ]，参数不足
                    g_parser.error_count++;
                    
                    // 立即响应错误
                    send_error_response("[ERROR]Too few parameters (expected 3)");
                    
                    // 重置状态机，等待下一个命令
                    reset_parser();
                }
            } else if (g_parser.char_count[g_parser.param_idx] < sizeof(g_parser.param_buf[0]) - 1) {
                // 正常参数字符
                g_parser.param_buf[g_parser.param_idx][g_parser.char_count[g_parser.param_idx]++] = data;
            } else {
                // 参数过长
                g_parser.error_count++;
                
                // ❌ 错误响应：参数过长
                send_error_response("[ERROR]Parameter too long (max 31 chars)");
                
                // 立即重置
                reset_parser();
            }
            break;
            
        case PARSE_ERROR:
            // 错误恢复：寻找下一个 '['
            if (data == '[') {
                reset_parser();
                g_parser.state = PARSE_TARGET;
                // 注意：不在这里发送错误消息，因为错误已在发生时报告
            }
            break;
            
        default:
            reset_parser();
            break;
    }
}

/**
 * @brief 检查是否有待处理的命令
 */
bool SerialCmd_IsReady(void)
{
    return g_parser.is_ready;
}

/**
 * @brief 应用待更新的 PID 参数（在主循环中调用）
 */
void SerialCmd_ApplyPendingParams(void)
{
    // 检查并应用每个 PID 的待更新参数
    if (PID_HasPendingUpdate(&anglePID)) {
        PID_ApplyPendingParams(&anglePID);
    }
    if (PID_HasPendingUpdate(&speedPID)) {
        PID_ApplyPendingParams(&speedPID);
    }
    if (PID_HasPendingUpdate(&turnPID)) {
        PID_ApplyPendingParams(&turnPID);
    }
}

/**
 * @brief 获取解析器句柄
 */
SerialCmdParser_t* SerialCmd_GetParser(void)
{
    return &g_parser;
}

// ============ 遥测数据上传实现 ============

/**
 * @brief 发送完整遥测数据
 */
void Telemetry_SendFullData(uint32_t timestamp)
{
    Serial_Printf(USART1,
        "%lu,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f,%.3f\r\n",
        (unsigned long)timestamp,
        // 直立环
        (float)anglePID.target,
        (float)anglePID.actual,
        (float)anglePID.out,
        // 速度环
        (float)speedPID.target,
        (float)speedPID.actual,
        (float)speedPID.out,
        // 转向环
        (float)turnPID.target,
        (float)turnPID.actual,
        (float)turnPID.out,
        // 直立环参数
        (float)anglePID.Kp,
        (float)anglePID.Ki,
        (float)anglePID.Kd
    );
}

/**
 * @brief 发送简化遥测数据
 */
void Telemetry_SendSimpleData(uint32_t timestamp)
{
    extern float angle;       // 当前角度（来自 control.c）
    extern int16_t left_PWM;  // 左轮 PWM（来自 system_config.h）
    
    Serial_Printf(USART1, "%lu,%.3f,%d,%.3f,%.3f,%.3f\r\n",
        (unsigned long)timestamp,
        (float)angle,
        (int)left_PWM,
        (float)anglePID.Kp,
        (float)anglePID.Ki,
        (float)anglePID.Kd
    );
}
