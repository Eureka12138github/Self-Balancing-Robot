/**
 * @file menu_data.c
 * @brief OLED菜单系统数据定义文件
 * 
 * 本文件定义了整个菜单系统的数据结构，包括菜单项和页面的配置。
 * 所有菜单数据都在此文件中集中管理，便于维护和扩展。
 * 
 * @author Eureka & Lingma
 * @date 2026-02-22
 */

#include "menu_data.h"
#include <stddef.h>
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

// ==================== 可编辑项配置（新增）====================
/**
 * @brief 示例：可编辑数值的配置参数
 * 
 * 定义在 Flash 中，节省 RAM
 * 可根据需要定义多个不同配置
 */
static const MenuEditConfig s_RP_edit_config = {
    .min  = -50,      // 最小值
    .max  = 50,    // 最大值
    .step = 1       // 每次调节步进
};


// ==================== 主菜单项定义 ====================
/**
 * @brief 主菜单项数组
 *
 * 定义主菜单页面的所有菜单项，作为系统功能的顶层入口。
 * 本数组混合使用完整初始化与简洁初始化方式：
 * - 第一项采用完整显式初始化，作为结构体字段用法的参考模板；
 * - 其余普通项采用位置+指定初始化混合方式，但显式设置关键字段 `.item_type` 以确保语义明确；
 * - 所有未显式初始化的成员（如滚动状态、编辑标志等）由编译器自动置零，符合预期行为。
 *
 * 支持的菜单项类型包括：
 * - 普通导航项：通过 `.submenu` 跳转至子菜单页面；
 * - 回调执行项：通过 `.callback` 执行特定功能（无子菜单）；
 * - 数值显示项：通过 `.u16_Value` 动态显示变量值（本数组未使用）；
 * - 长文本项：文本宽度超过屏幕时自动触发水平滚动（需运行时计算宽度）。
 *
 * ⚠️ 注意：
 * - 数组必须以全零项 `{0}` 结尾，作为遍历终止标记；
 * - 若新增字段到 `MyMenuItem` 结构体，请同步更新第一项的完整初始化以保持示例有效性。
 */
MyMenuItem MainItems[] = {    
    // 【完整初始化示例】—— 明确所有字段，便于理解结构体布局与默认值
    {
        .text         = "监控",               ///< 显示文本
        .callback     = NULL,                ///< 无回调函数
        .submenu      = &MonitorPage,        ///< 跳转至监控子菜单
        .int16_Value    = NULL,                ///< 不关联数值变量
        .item_type    = MENU_ITEM_NORMAL,    ///< 普通菜单项
        .edit_config  = NULL,                ///< 不可编辑
        .is_editing   = false,               ///< 初始非编辑状态
        .scroll_offset= 0,                   ///< 滚动偏移初始为0
        .text_width   = 0,                   ///< 文本宽度由运行时计算
        .is_scrolling = false                ///< 初始无滚动动画
    },

    // 普通导航项：简洁写法，但显式指定 item_type 保证语义安全
    {"设置", NULL, &SettingsPage, NULL, MENU_ITEM_NORMAL},          ///< 导航到设置子菜单
    {"更多", NULL, &MorePage, NULL, MENU_ITEM_NORMAL},              ///< 导航到更多子菜单

    // 回调执行项：点击后直接执行函数，不跳转页面
    {"TEST1", Test_Callback_1, NULL, NULL, MENU_ITEM_NORMAL},       ///< 执行测试回调函数

    // 长文本项：文本超宽时自动启用水平滚动（滚动逻辑在渲染时处理）
    {"TEST2:ABCDIHOIAJFJLSAJDKFJAKLJDLFAJLD", NULL, NULL, NULL, MENU_ITEM_NORMAL},
    {"TEST3:ABCDIHOIAJFJLSAJDKFJAKLJDLFAJLD", NULL, NULL, NULL, MENU_ITEM_NORMAL},
    {"TEST4:ABCDIHOIAJFJLSAJDKFJAKLJDLFAJLD", NULL, NULL, NULL, MENU_ITEM_NORMAL},

    // 数组结束标记：必须存在，用于菜单遍历时判断终止条件
    {0}
};

// ==================== 设置子菜单项定义 ====================
/**
 * @brief 设置子菜单项数组
 * 
 * 演示基本的菜单项配置方式
 * 注意最后一项为返回按钮的特殊配置
 */
MyMenuItem SettingsItems[] = {
    {"Setting_1", NULL, NULL,NULL, MENU_ITEM_NORMAL},              // 普通设置项
    {"Setting_2", NULL, NULL, NULL,MENU_ITEM_NORMAL},              // 普通设置项
    {"Setting_3", NULL, NULL, NULL,MENU_ITEM_NORMAL},              // 普通设置项	
    // 返回项配置 - 特殊类型MENU_ITEM_BACK
    {"[返回]", NULL, NULL, NULL, MENU_ITEM_BACK},  
    
    // 数组结束标记
    {0}
};

// ==================== 监控子菜单项定义 ====================
/**
 * @brief 监控子菜单项数组
 * 
 * 演示数值显示和回调函数的使用方法
 * 数值项会自动显示关联变量的当前值
 */
MyMenuItem MonitorItems[] = {

    {
        .text        = "RP1:%d",
        .callback    = NULL,                      // 编辑模式下无需回调
        .submenu     = NULL,
        .int16_Value   = &g_rp_value1,              // 关联变量
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_RP_edit_config, // 启用编辑
        .is_editing  = false,                     // 初始非编辑态
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
	
    {
        .text        = "RP2:%d",
        .callback    = NULL,                      // 编辑模式下无需回调
        .submenu     = NULL,
        .int16_Value   = &g_rp_value2,              // 关联变量
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_RP_edit_config, // 启用编辑
        .is_editing  = false,                     // 初始非编辑态
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "RP3:%d",
        .callback    = NULL,                      // 编辑模式下无需回调
        .submenu     = NULL,
        .int16_Value   = &g_rp_value3,              // 关联变量
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_RP_edit_config, // 启用编辑
        .is_editing  = false,                     // 初始非编辑态
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },	
	
    {
        .text        = "RP4:%d",
        .callback    = NULL,                      // 编辑模式下无需回调
        .submenu     = NULL,
        .int16_Value   = &g_rp_value4,              // 关联变量
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_RP_edit_config, // 启用编辑
        .is_editing  = false,                     // 初始非编辑态
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },		

    // 返回项
    {"[返回]", NULL, NULL, NULL, MENU_ITEM_BACK},  
    
    // 数组结束标记
    {0}
};

// ==================== 更多子菜单项定义 ====================
/**
 * @brief 更多子菜单项数组
 * 
 * 简单的信息展示菜单
 */
MyMenuItem MoreItems[] = {
    {"About", NULL, NULL, NULL, MENU_ITEM_NORMAL},                  // 关于信息
    {"Help", NULL, NULL, NULL, MENU_ITEM_NORMAL},                   // 帮助信息
    
    // 返回项
    {"[返回]", NULL, NULL, NULL, MENU_ITEM_BACK},  
    
    // 数组结束标记
    {0}
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
    .scroll_delay = SET_SCROLL_DELAY,             // 滚动动画间隔
	.last_encoder_time = 0,
	.encoder_accel = 1	
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
    .scroll_delay = SET_SCROLL_DELAY,             // 滚动动画间隔
	.last_encoder_time = 0,
	.encoder_accel = 1	
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
    .scroll_delay = SET_SCROLL_DELAY,             // 滚动动画间隔
	.last_encoder_time = 0,
	.encoder_accel = 1
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
    .scroll_delay = SET_SCROLL_DELAY,             // 滚动动画间隔
	.last_encoder_time = 0,
	.encoder_accel = 1	
};
