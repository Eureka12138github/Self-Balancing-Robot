/**
 * @file my_menu_data.c
 * @brief OLED菜单系统数据定义文件
 * 
 * 本文件定义了整个菜单系统的数据结构，包括菜单项和页面的配置。
 * 所有菜单数据都在此文件中集中管理，便于维护和扩展。
 * 
 * @author Eureka & Lingma
 * @date 2026-02-22
 */

#include "my_menu_data.h"
#include <stddef.h>
#include "sensors_data.h"
#include "menu_callbacks.h"
#include "system_config.h"

/*
 * ==================== 使用说明 ====================
 * 
 * 1. 菜单项(MyMenuItem)结构说明：
 *    - text: 显示的文本内容，支持格式化字符串(%d等)
 *    - callback: 按键确认时执行的回调函数指针
 *    - submenu: 子菜单页面指针，用于页面跳转
 *    - u16_Value: 关联的16位数值变量指针，用于动态显示
 *    - item_type: 菜单项类型(普通项/返回项)
 * 
 * 2. 页面(MyMenuPage)结构说明：
 *    - parent: 父页面指针，用于返回操作
 *    - items: 指向菜单项数组
 *    - active_id: 当前活动项的索引
 *    - visible_start: 可视区域起始项索引
 *    - max_visible: 最大可见项数
 *    - slot: 当前光标所在的槽位(0-max_visible-1)
 *    - ItemNum: 菜单项总数(自动计算)
 *    - last_scroll_time: 滚动动画上次更新时间
 *    - scroll_delay: 滚动动画更新间隔(ms)
 * 
 * 3. 配置要点：
 *    - 每个菜单项数组必须以{NULL, NULL, NULL, NULL}结尾
 *    - ItemNum通过sizeof自动计算，无需手动维护
 *    - 返回项需设置item_type为MENU_ITEM_BACK
 *    - 数值显示项需要提供有效的u16_Value指针
 * ================================================
 */

// ==================== 主菜单项定义 ====================
/**
 * @brief 主菜单项数组
 * 
 * 包含系统的主要功能入口，支持多种类型的菜单项：
 * - 普通导航项：指向子菜单页面
 * - 回调执行项：执行特定功能函数
 * - 数值显示项：动态显示传感器数据
 * - 长文本项：自动启用水平滚动显示
 */
MyMenuItem MainItems[] = {    
    {"监控", NULL, &MonitorPage, NULL},           // 导航到监控子菜单
    {"设置", NULL, &SettingsPage, NULL},          // 导航到设置子菜单
    {"更多", NULL, &MorePage, NULL},              // 导航到更多子菜单
    {"其他", NULL, NULL, NULL},                   // 普通项(无功能)
    {"TEST1", Test_Callback_1, NULL, NULL},       // 回调执行项
    {"TEST2", Test_Callback_2, NULL, NULL},       // 回调执行项
    {"TEST3", Test_Callback_3, NULL, NULL},       // 回调执行项
    
    // 长文本项示例 - 当文本超过屏幕宽度时自动滚动
    {"TEST4:ABCDIHOIAJFJLSAJDKFJAKLJDLFAJLD", NULL, NULL, NULL}, 
    {"TEST5:ABCDIHOIAJFJLSAJDKFJAKLJDLFAJLD", NULL, NULL, NULL},
    {"TEST6:ABCDIHOIAJFJLSAJDKFJAKLJDLFAJLD", NULL, NULL, NULL},
    
    {"TEST7", NULL, NULL, NULL},
    {"TEST8", NULL, NULL, NULL},
    {"TEST9", NULL, NULL, NULL},
    {"TEST10", NULL, NULL, NULL},    
    
    // 数组结束标记 - 必须存在
    {NULL, NULL, NULL, NULL}
};

// ==================== 设置子菜单项定义 ====================
/**
 * @brief 设置子菜单项数组
 * 
 * 演示基本的菜单项配置方式
 * 注意最后一项为返回按钮的特殊配置
 */
MyMenuItem SettingsItems[] = {
    {"Setting_1", NULL, NULL, NULL},              // 普通设置项
    {"Setting_1", NULL, NULL, NULL},              // 普通设置项
    {"Setting_1", NULL, NULL, NULL},              // 普通设置项
    
    // 返回项配置 - 特殊类型MENU_ITEM_BACK
    {"[返回]", NULL, NULL, NULL, MENU_ITEM_BACK},  
    
    // 数组结束标记
    {NULL, NULL, NULL, NULL}
};

// ==================== 监控子菜单项定义 ====================
/**
 * @brief 监控子菜单项数组
 * 
 * 演示数值显示和回调函数的使用方法
 * 数值项会自动显示关联变量的当前值
 */
MyMenuItem MonitorItems[] = {
    // 数值显示项 - 显示温度数据
    {"温度:%d `C", NULL, NULL, &g_sensor_data.temp},
    
    // 数值显示项 - 显示湿度数据  
    {"湿度:%d %%RH", NULL, NULL, &g_sensor_data.humi},
    
    // 回调+数值项 - 执行函数并显示数值
    {"XXXX-XXXX->:%d", Test_Callback_4, NULL, &g_simulate_sensor_data},
    
    // 返回项
    {"[返回]", NULL, NULL, NULL, MENU_ITEM_BACK},  
    
    // 数组结束标记
    {NULL, NULL, NULL, NULL}
};

// ==================== 更多子菜单项定义 ====================
/**
 * @brief 更多子菜单项数组
 * 
 * 简单的信息展示菜单
 */
MyMenuItem MoreItems[] = {
    {"About", NULL, NULL, NULL},                  // 关于信息
    {"Help", NULL, NULL, NULL},                   // 帮助信息
    
    // 返回项
    {"[返回]", NULL, NULL, NULL, MENU_ITEM_BACK},  
    
    // 数组结束标记
    {NULL, NULL, NULL, NULL}
};

// ==================== 页面定义 ====================
/**
 * @brief 主页面定义
 * 
 * 页面配置的关键要点：
 * 1. ItemNum通过sizeof自动计算，确保准确性
 * 2. scroll_delay控制滚动动画速度(毫秒)
 * 3. last_scroll_time初始化为0确保正确启动
 * 4. parent设为NULL表示顶层页面
 */
MyMenuPage MainPage = {
    .parent = NULL,                               // 顶层页面，无父页面
    .items = MainItems,                          // 关联主菜单项数组
    .active_id = 0,                              // 默认选中第一项
    .visible_start = 0,                          // 从第一项开始显示
    .max_visible = 4,                            // 最多同时显示4项（具体能显示多少项最终取决于屏幕高度和字体大小）
    .slot = 0,                                   // 光标在第一个槽位
    .ItemNum = (sizeof(MainItems) / sizeof(MyMenuItem)) - 1,  // 自动计算项数
    .last_scroll_time = 0,                       // 滚动时间基准初始化
    .scroll_delay = SET_SCROLL_DELAY             // 滚动动画间隔
};

/**
 * @brief 设置页面定义
 * 
 * 子页面配置要点：
 * 1. parent指向父页面，用于返回操作
 * 2. 其他配置与主页面类似
 */
MyMenuPage SettingsPage = {
    .parent = &MainPage,                         // 返回到主页面
    .items = SettingsItems,                      // 关联设置菜单项
    .active_id = 0,                              // 默认选中第一项
    .visible_start = 0,                          // 从第一项开始显示
    .max_visible = 4,                            // 最多同时显示4项
    .slot = 0,                                   // 光标在第一个槽位
    .ItemNum = (sizeof(SettingsItems) / sizeof(MyMenuItem)) - 1,  // 自动计算
    .last_scroll_time = 0,                       // 滚动时间基准初始化
    .scroll_delay = SET_SCROLL_DELAY             // 滚动动画间隔
};

/**
 * @brief 监控页面定义
 * 
 * 包含数值显示功能的页面配置
 */
MyMenuPage MonitorPage = {
    .parent = &MainPage,                         // 返回到主页面
    .items = MonitorItems,                       // 关联监控菜单项
    .active_id = 0,                              // 默认选中第一项
    .visible_start = 0,                          // 从第一项开始显示
    .max_visible = 4,                            // 最多同时显示4项
    .slot = 0,                                   // 光标在第一个槽位
    .ItemNum = (sizeof(MonitorItems) / sizeof(MyMenuItem)) - 1,   // 自动计算
    .last_scroll_time = 0,                       // 滚动时间基准初始化
    .scroll_delay = SET_SCROLL_DELAY             // 滚动动画间隔
};

/**
 * @brief 更多页面定义
 * 
 * 简单信息展示页面配置
 */
MyMenuPage MorePage = {
    .parent = &MainPage,                         // 返回到主页面
    .items = MoreItems,                          // 关联更多菜单项
    .active_id = 0,                              // 默认选中第一项
    .visible_start = 0,                          // 从第一项开始显示
    .max_visible = 4,                            // 最多同时显示4项
    .slot = 0,                                   // 光标在第一个槽位
    .ItemNum = (sizeof(MoreItems) / sizeof(MyMenuItem)) - 1,      // 自动计算
    .last_scroll_time = 0,                       // 滚动时间基准初始化
    .scroll_delay = SET_SCROLL_DELAY             // 滚动动画间隔
};
