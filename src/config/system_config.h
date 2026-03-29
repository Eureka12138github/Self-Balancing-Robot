// system_config.h
#ifndef SYSTEM_CONFIG_H
#define SYSTEM_CONFIG_H
#include <stdint.h>
#include <stdbool.h>
#include "PID.h"
#include "kalman.h"

/*============================================================================
 *                          OLED MENU 配置
 *============================================================================*/
/** @defgroup OLED_MENU_Config */
/** @{ */
/** @brief OLED屏幕宽度(像素) */
#define OLED_WIDTH     128

/** @brief OLED屏幕高度(像素) */
#define OLED_HEIGHT    64

#define OLED_PAGES    (OLED_HEIGHT / 8)

/** @brief 字体宽度(像素) */
#define FONT_WIDTH            16

/** @brief 字体高度(像素) */
#define FONT_HEIGHT           16

/** @brief 行间距(像素) */
#define LINE_SPACING          4

/** @brief 底部安全边距(像素) */
#define SAFE_MARGIN_BOTTOM    4

/** @brief 单行总高度(字体高度+行间距) */
#define LINE_HEIGHT           (FONT_HEIGHT + LINE_SPACING)

/** @brief 屏幕最大安全可见行数 */
#define MAX_SAFE_VISIBLE_ITEMS ((OLED_HEIGHT - SAFE_MARGIN_BOTTOM) / LINE_HEIGHT)

/** @brief 菜单文本显示起始点X坐标(像素) */
#define START_POINT_OF_TEXT_DISPLAY 20

/** @brief 文本滚动动画更新间隔(毫秒) */
#define SET_SCROLL_DELAY 16  

/** @brief UI主循环帧间隔(毫秒) */
#define SET_FRAME_INTERVAL 33

/*
 * 滚动速度配置参考方案：
 * 
 * 方案1：高精度滚动（推荐）
 * page->scroll_delay = 16;   // 60FPS滚动更新
 * frame_interval = 33;       // 30FPS显示刷新
 * 优点：滚动平滑，显示负载适中
 *
 * 方案2：同步更新（最稳定）
 * page->scroll_delay = 33;   // 与显示同步
 * frame_interval = 33;       // 完全同步
 * 优点：绝对稳定，但滚动颗粒度较大
 *
 * 方案3：节能模式
 * page->scroll_delay = 66;   // 降低滚动频率
 * frame_interval = 66;       // 降低整体刷新率
 * 优点：省电，适合电池供电设备
 */
 
/** @} */

/*============================================================================
 *                      系统级全局变量声明（跨模块共享）
 *============================================================================*/

/** @brief 系统运行状态标志 */
extern bool run_flag;

/** @brief PID 控制器参数 */
extern PID_t anglePID;
extern PID_t speedPID;
extern PID_t turnPID;

/** @brief 传感器校准参数 */
extern int16_t g_gyro_y_offset;      // Gyro Y-axis bias
extern float g_angleAcc_offset;      // Accelerometer angle offset

/** @brief 卡尔曼滤波器实例 */
extern kalman_t g_kalman;            // Kalman filter for sensor fusion

/*============================================================================
 *                      平衡控制算法配置参数
 *============================================================================*/
/** @defgroup Balance_Control_Params 平衡控制参数 */
/** @{ */

/** @brief 互补滤波系数 (0~1, 越小越信任加速度计) */
#define COMPLEMENTARY_FILTER_ALPHA    0.01f

/** @brief 倒地保护角度阈值 (度) */
#define ANGLE_FALL_THRESHOLD          50.0f

/** @brief 电机参数：极对数 */
#define MOTOR_POLE_PAIRS              44      // 电机极对数

/** @brief 电机参数：编码器读取周期 (秒) */
#define ENCODER_READ_PERIOD           0.04f   // 编码器读取周期 (s)

/** @brief 电机参数：减速比 */
#define GEAR_RATIO                    9.27666f // 减速比

/** @brief 速度转换系数 = 极对数 × 读取周期 × 减速比 */
#define SPEED_CONVERSION_FACTOR       (MOTOR_POLE_PAIRS * ENCODER_READ_PERIOD * GEAR_RATIO)
// 计算结果：44 × 0.04 × 9.27666 ≈ 16.326816

/** @brief 弧度转角度系数 = 180/π */
#define RAD_TO_DEG                    57.2958f

/** @brief 陀螺仪标度因子 = 量程 × 周期 / 32768
 * 计算：2000(°/s) × 0.005(s) / 32768 ≈ 0.000305176
 */
#define GYRO_SCALE_FACTOR             0.000305176f

/** @} */

#endif
