/**
 * @file task_sched.c
 * @brief 基于时间片检查的轻量级非阻塞任务管理器
 * 
 * @details
 * 【重要架构说明】
 * 本模块 **不是** 严格意义上的“时间片轮转 (Round-Robin)”系统。
 * - 真正的 Round-Robin 通常指操作系统级别的“抢占式”调度，能强制中断当前任务。
 * - 本模块实质是一个 **“非阻塞式主循环管理器” (Non-blocking Main Loop Manager)** 或 **“时间片检查器”**。
 * 
 * 【工作原理】
 * 1. 依赖定时器中断中定期（此刻需5ms调用一次）调用 `TaskSchedule()` 来递减时间计数器。
 * 2. 当计数器归零，标记任务为“可执行 (run=1)”。
 * 3. `TaskHandler()` 在主循环中顺序执行所有被标记的任务。
 * 
 * 【局限性警告】
 * - **非实时性**：任务的实际执行间隔 = 设定周期 + 主循环中其他任务的耗时总和。
 * - **阻塞风险**：若列表中某个任务（如 UI 刷新）耗时过长，后续所有任务都会发生“堆积延迟”。
 * - **适用场景**：按键扫描、LED 闪烁、低速传感器读取、数据上报等 **软实时 (Soft Real-Time)** 任务。
 * - **禁止场景**：严禁用于 PID 控制、电机 PWM 生成、高速编码器采集等对时序抖动敏感的 **硬实时 (Hard Real-Time)** 任务（此类任务应放入定时器中断）。
 */

/* ======================== 头文件包含 ======================== */
#include "task_sched.h"
#include "system_config.h"
#include <stdbool.h>
#include "key.h"
#include "hall_encoder.h"

/* ======================== 宏定义 ======================== */
// 自动计算任务表长度，避免手动修改出错
#define TASK_NUM_MAX        (sizeof(TaskComps) / sizeof(TaskComps[0]))  

// 定时任务周期配置（单位：调用次数，需结合 TaskSchedule 的调用频率换算）
// 假设 TaskSchedule 每 5ms 被调用一次：
#define KEY_SCAN_INTERVEL_MS           4   ///< 按键扫描周期：4 * 5ms = 20ms

/* ======================== 类型定义 ======================== */

/**
 * @brief 任务控制块 (TCB) - 简化版
 */
typedef struct {
    bool run;                   ///< 任务就绪标志：1=可执行，0=等待
    uint16_t TimCount;          ///< 当前倒计时计数值
    uint16_t TimeRload;         ///< 重载值（周期基数）
    void (*pTaskFunc)(void);    ///< 任务函数指针
} TaskComps_t;

/* ======================== 定时任务函数 ======================== */


/* ======================== 任务表定义 ======================== */

/** 
 * @brief 静态任务列表
 * @note 任务执行顺序即为数组排列顺序。耗时长的任务建议放在后面，以免阻塞前面的任务。
 */
static TaskComps_t TaskComps[] = {
    // {初始run标志，当前计数，重载周期，函数指针}
    {0, KEY_SCAN_INTERVEL_MS,      KEY_SCAN_INTERVEL_MS,      Key_Scan},
};

/* ======================== 调度器实现 ======================== */

/**
 * @brief 时间片更新器 (Time-Slice Checker)
 * 
 * @details
 * 定时器中断高频调用（例如每 5ms 调用一次）。
 * 它不负责“切换”任务，只负责“倒计时”和“打旗语”。
 * 
 * @warning 
 * 若主循环被阻塞（如进入长延时或死循环），本函数无法执行，所有任务计时将暂停。
 */
void TaskSchedule(void) {
    for (uint8_t i = 0; i < TASK_NUM_MAX; i++) {
        if (TaskComps[i].TimCount > 0) {
            TaskComps[i].TimCount--;

            // 计数归零，重置计数器并置位运行标志
            if (TaskComps[i].TimCount == 0) {
                TaskComps[i].TimCount = TaskComps[i].TimeRload;
                TaskComps[i].run = 1;  
            }
        }
    }
}

/**
 * @brief 任务执行器 (Cooperative Dispatcher)
 * 
 * @details
 * 顺序遍历任务表，执行所有就绪任务。
 * 此为 **协作式 (Cooperative)** 执行：一旦某个 pTaskFunc() 内部耗时过长，
 * 后续任务必须等待其返回后才能执行，导致实际周期偏离设定值。
 * 
 * @best_practice
 * 确保每个 pTaskFunc 内部都是非阻塞的（无 delay，无长循环），快速返回。
 */
void TaskHandler(void) {
    for (uint8_t i = 0; i < TASK_NUM_MAX; i++) {
        // 检查标志位且函数指针有效
        if (TaskComps[i].run && TaskComps[i].pTaskFunc) {
            TaskComps[i].run = 0;       // 立即清除标志，防止重入
            TaskComps[i].pTaskFunc();   // 执行任务（此处可能发生阻塞）
        }
    }
}

/* ======================== 系统辅助函数 ======================== */
// 预留接口：可扩展任务动态添加、挂起、恢复等功能
