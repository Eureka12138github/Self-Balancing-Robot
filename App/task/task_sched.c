/**
 * @file task_sched.c
 * @brief 基于时间片轮转 + DMA事件驱动的轻量级任务调度系统
 *
 * 本模块实现两类任务：
 * 1. 定时周期任务（由 SysTick 驱动 TaskSchedule 更新）
 * 2. DMA 事件触发任务（由 DMA 中断标志驱动 DMATaskHandler 执行）
 */

/* ======================== 头文件包含 ======================== */
#include "task_sched.h"
#include "stdbool.h"
#include "dht11.h"
#include "system_config.h"
#include "alarm.h"
#include "sensors_data.h"
#include "key.h"


/* ======================== 宏定义 ======================== */
// 任务数组大小计算
#define TASK_NUM_MAX        (sizeof(TaskComps) / sizeof(TaskComps[0]))  ///< 定时任务数量

// 定时任务周期（单位：ms）
#define DHT11_READ_INTERVAL_MS          1000  ///< DHT11读取间隔
#define LED_FLASH_INTERVAL_MS           500   ///< LED闪烁间隔
#define READ_TIME_INTERVAL_MS           500   ///< RTC时间读取间隔
#define KEY_SCAN_INTERVEL_MS           20   ///< 按键扫描周期



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

void DHT11_Read(void) {
    static u8 DHT11_Error_Flag = 0;
    u16 temp = 0, humi = 0;
    if (DHT11_Read_Data(&temp, &humi)) {
        SensorsData_Update_Temp_Humi(&temp, &humi);
        DHT11_Error_Flag = 0;
    } else {
        if (++DHT11_Error_Flag >= 5) {
			// 暂时不处理
        }
    }
}




/* ======================== 任务表定义 ======================== */

/** @brief 定时任务表 */
static TaskComps_t TaskComps[] = {

    {0, DHT11_READ_INTERVAL_MS,      DHT11_READ_INTERVAL_MS,      DHT11_Read},
	{0, KEY_SCAN_INTERVEL_MS,      KEY_SCAN_INTERVEL_MS,            Key_Scan},
	
//    {0, LED_FLASH_INTERVAL_MS,       LED_FLASH_INTERVAL_MS,       Led_Turn},


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
