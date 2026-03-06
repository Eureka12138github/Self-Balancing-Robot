/**
 * @file task_sched.c
 * @brief 基于时间片轮转 + DMA事件驱动的轻量级任务调度系统
 *
 * 本模块实现任务：
 * 定时周期任务（由 SysTick 驱动 TaskSchedule 更新）
 */

/* ======================== 头文件包含 ======================== */
#include "task_sched.h"
#include "system_config.h"
#include <stdbool.h>
#include "key.h"
#include "hall_encoder.h"

/* ======================== 宏定义 ======================== */
// 任务数组大小计算
#define TASK_NUM_MAX        (sizeof(TaskComps) / sizeof(TaskComps[0]))  ///< 定时任务数量

// 定时任务周期（单位：ms）
#define KEY_SCAN_INTERVEL_MS           20   ///< 按键扫描周期
#define HALL_ENCODER_GET_INTERVEL_MS           40   ///< 霍尔编码器增量获取周期


/* ======================== 类型定义 ======================== */

/**
 * @brief 任务调度结构体（时间驱动）
 */
typedef struct {
    bool run;                   ///< 是否可执行
    uint16_t TimCount;          ///< 当前计数值
    uint16_t TimeRload;         ///< 重载值（周期）
    void (*pTaskFunc)(void);    ///< 任务函数指针
} TaskComps_t;




/* ======================== 定时任务函数 ======================== */

void Hall_Encoder_Test(void) {
	g_rp_value3 = Hall_Encoder_Get(ENCODER_LEFT);
	g_rp_value4 = Hall_Encoder_Get(ENCODER_RIGHT);

}


/* ======================== 任务表定义 ======================== */

/** @brief 定时任务表 */
static TaskComps_t TaskComps[] = {
	
	{0, KEY_SCAN_INTERVEL_MS,      KEY_SCAN_INTERVEL_MS, Key_Scan},
	{0, HALL_ENCODER_GET_INTERVEL_MS,      HALL_ENCODER_GET_INTERVEL_MS, Hall_Encoder_Test},


};


/* ======================== 调度器实现 ======================== */

/**
 * @brief 定时任务调度器
 *
 * 更新所有定时任务的时间计数器。
 * 当计数归零时，重载计数值并标记任务为可执行（run = 1）。
 */
void TaskSchedule(void) {
    for (u8 i = 0; i < TASK_NUM_MAX; i++) {
        if (TaskComps[i].TimCount > 0) {
            TaskComps[i].TimCount--;

            // 时间到，重载并标记可执行
            if (TaskComps[i].TimCount == 0) {
                TaskComps[i].TimCount = TaskComps[i].TimeRload;
                TaskComps[i].run = 1;  // 所有任务默认可执行
            }
        }
    }
}

/**
 * @brief 定时任务执行器
 *
 * 遍历任务表，执行所有标记为可执行的任务。
 */
void TaskHandler(void) {
    for (u8 i = 0; i < TASK_NUM_MAX; i++) {
        if (TaskComps[i].run && TaskComps[i].pTaskFunc) {
            TaskComps[i].run = 0;
            TaskComps[i].pTaskFunc();
        }
    }
}


/* ======================== 系统辅助函数 ======================== */

// ...
