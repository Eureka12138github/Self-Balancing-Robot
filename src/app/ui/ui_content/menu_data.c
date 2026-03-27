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
#include "control.h"  // 新增：用于获取监控数据

// ========== 监控数据包装器（用于 UI 显示） ==========
// 这些变量在每次菜单刷新时更新，提供对 control.c static 变量的访问

static float g_monitor_angle = 0.0f;
static float g_monitor_angleAcc = 0.0f;
static float g_monitor_dif_speed = 0.0f;

/**
 * @brief 更新监控数据（在菜单主循环中调用）
 */
void Update_Monitor_Data(void) {
    BalanceMonitor_t monitor;
    Balance_GetMonitorData(&monitor);
    
    g_monitor_angle = monitor.angle;
    g_monitor_angleAcc = monitor.angleAcc;
    g_monitor_dif_speed = monitor.dif_speed;
}

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

// ==================== Edit Configs ====================
/**
 * @brief PID parameter edit configs (stored in Flash, save RAM)
 * 
 * 直立环 PID 参数调试范围建议：
 * - Kp: -20 ~ 20, step=1 (实际范围 0~20，负值用于调试)
 * - Ki: -2 ~ 2, step=0.1 (直立环通常 Ki=0)
 * - Kd: -20 ~ 50, step=1 (实际范围 0~50)
 * 
 * 调试步骤：
 * 1. 先调 Kp：从 3.0 开始，每次 +0.5，直到出现小幅振荡
 * 2. 再调 Kd：从 15 开始，每次 +2，直到振荡消失
 * 3. 最后微调：Kp 和 Kd 配合，找到最佳平衡点
 */
/**
 * @brief PID parameter edit configs (stored in Flash, save RAM)
 */
static const MenuEditConfig s_RP_edit_config = {
    .min  = -50,
    .max  = 50,
    .step = 1
};

static const MenuEditConfig s_PID_Kp_edit_config = {
    .min  = -20,
    .max  = 20,
    .step = 1
};

static const MenuEditConfig s_PID_Ki_edit_config = {
    .min  = -2,
    .max  = 2,
    .step = 1
};

static const MenuEditConfig s_PID_Kd_edit_config = {
    .min  = -20,
    .max  = 50,
    .step = 1
};

static const MenuEditConfig s_PID_Target_edit_config = {
    .min  = -1000,
    .max  = 1000,
    .step = 1
};

// ==================== Turn PID Submenu Items ====================
/**
 * @brief Turn PID parameters (6 params + back)
 */
MyMenuItem TurnPIDItems[] = {
    {
        .text        = "T_Kp:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,              // 明确设置为 NULL
        .float_Value = &turnPID.Kp,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_PID_Kp_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "T_Ki:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &turnPID.Ki,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_PID_Ki_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "T_Kd:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &turnPID.Kd,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_PID_Kd_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "T_tar:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &turnPID.target,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_PID_Target_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "T_act:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &g_monitor_dif_speed,  // 使用包装器变量
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = NULL,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "T_out:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &turnPID.out,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = NULL,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {"[返回]", NULL, NULL, NULL, NULL, MENU_ITEM_BACK},
    {0}
};

MyMenuPage TurnPIDPage = {
    .parent = &SettingsPage,
    .items = TurnPIDItems,
    .active_id = 0,
    .visible_start = 0,
    .max_visible = 4,
    .slot = 0,
    .ItemNum = (sizeof(TurnPIDItems) / sizeof(MyMenuItem)) - 1,
    .last_scroll_time = 0,
    .scroll_delay = SET_SCROLL_DELAY,
    .last_input_time = 0,
    .input_accel = 1
};

// ==================== Speed PID Submenu Items ====================
/**
 * @brief Speed PID parameters (6 params + back)
 */
MyMenuItem SpeedPIDItems[] = {
    {
        .text        = "S_Kp:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &speedPID.Kp,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_PID_Kp_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "S_Ki:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &speedPID.Ki,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_PID_Ki_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "S_Kd:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &speedPID.Kd,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_PID_Kd_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "S_tar:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &speedPID.target,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_PID_Target_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "S_act:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &speedPID.actual,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = NULL,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "S_out:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &speedPID.out,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = NULL,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {"[返回]", NULL, NULL, NULL, NULL, MENU_ITEM_BACK},
    {0}
};

MyMenuPage SpeedPIDPage = {
    .parent = &SettingsPage,
    .items = SpeedPIDItems,
    .active_id = 0,
    .visible_start = 0,
    .max_visible = 4,
    .slot = 0,
    .ItemNum = (sizeof(SpeedPIDItems) / sizeof(MyMenuItem)) - 1,
    .last_scroll_time = 0,
    .scroll_delay = SET_SCROLL_DELAY,
    .last_input_time = 0,
    .input_accel = 1
};

// ==================== Angle PID Submenu Items ====================
/**
 * @brief Angle PID parameters (6 params + back)
 */
MyMenuItem AnglePIDItems[] = {
    {
        .text        = "A_Kp:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &anglePID.Kp,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_PID_Kp_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "A_Ki:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &anglePID.Ki,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_PID_Ki_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "A_Kd:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &anglePID.Kd,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_PID_Kd_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "A_tar:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &anglePID.target,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_PID_Target_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "A_act:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &g_monitor_angle,  // 使用包装器变量
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = NULL,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "A_out:%+4.3f",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = &anglePID.out,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = NULL,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {"[返回]", NULL, NULL, NULL, NULL, MENU_ITEM_BACK},
    {0}
};

MyMenuPage AnglePIDPage = {
    .parent = &SettingsPage,
    .items = AnglePIDItems,
    .active_id = 0,
    .visible_start = 0,
    .max_visible = 4,
    .slot = 0,
    .ItemNum = (sizeof(AnglePIDItems) / sizeof(MyMenuItem)) - 1,
    .last_scroll_time = 0,
    .scroll_delay = SET_SCROLL_DELAY,
    .last_input_time = 0,
    .input_accel = 1
};

// ==================== Main Menu Items ====================
/**
 * @brief Main menu items array
 */
MyMenuItem MainItems[] = {    
    // 【完整初始化示例】—— 明确所有字段，便于理解结构体布局与默认值
    {"设置", NULL, &SettingsPage, NULL, MENU_ITEM_NORMAL},          ///< 导航到设置子菜单   


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

    {"更多", NULL, &MorePage, NULL, MENU_ITEM_NORMAL},              ///< 导航到更多子菜单




    // 数组结束标记：必须存在，用于菜单遍历时判断终止条件
    {0}
};

// ==================== Settings Menu Items ====================
/**
 * @brief Settings menu items (with submenu entries)
 */
MyMenuItem SettingsItems[] = {
    // 基础功能
    {
        .text         = "RUN/STOP",
        .callback     = Test_Callback_1,
        .submenu      = NULL,
        .int16_Value  = NULL,
        .item_type    = MENU_ITEM_NORMAL,
        .edit_config  = NULL,
        .is_editing   = false,
        .scroll_offset= 0,
        .text_width   = 0,
        .is_scrolling = false
    },
	
    // PID 参数分组入口（每个入口进入一个子页面）
    {
        .text         = "Turn PID",
        .callback     = NULL,
        .submenu      = &TurnPIDPage,    // 指向转向环 PID 子页面
        .int16_Value  = NULL,
        .item_type    = MENU_ITEM_NORMAL,
        .edit_config  = NULL,
        .is_editing   = false,
        .scroll_offset= 0,
        .text_width   = 0,
        .is_scrolling = false
    },
    {
        .text         = "Speed PID",
        .callback     = NULL,
        .submenu      = &SpeedPIDPage,   // 指向速度环 PID 子页面
        .int16_Value  = NULL,
        .item_type    = MENU_ITEM_NORMAL,
        .edit_config  = NULL,
        .is_editing   = false,
        .scroll_offset= 0,
        .text_width   = 0,
        .is_scrolling = false
    },
    {
        .text         = "Angle PID",
        .callback     = NULL,
        .submenu      = &AnglePIDPage,   // 指向直立环 PID 子页面
        .int16_Value  = NULL,
        .item_type    = MENU_ITEM_NORMAL,
        .edit_config  = NULL,
        .is_editing   = false,
        .scroll_offset= 0,
        .text_width   = 0,
        .is_scrolling = false
    },

    // 返回项
    {"[返回]", NULL, NULL, NULL, NULL, MENU_ITEM_BACK},  
    
    // 数组结束标记
    {0}
};

// ==================== Monitor Menu Items ====================
/**
 * @brief Monitor menu items (with numeric display)
 */
MyMenuItem MonitorItems[] = {

    {
        .text        = "Acc:%+4.3f",
        .callback    = NULL,                      // 编辑模式下无需回调
        .submenu     = NULL,
        .int16_Value = NULL,              // 明确设置为 NULL
        .float_Value = &g_monitor_angleAcc,         // 关联包装器变量
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = NULL,
        .is_editing  = false,                     // 初始非编辑态
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "gy:?",
        .callback    = NULL,                      // 编辑模式下无需回调
        .submenu     = NULL,
        .int16_Value = NULL,              // 关联变量
        .float_Value = NULL,             // 明确设置为 NULL
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = NULL,
        .is_editing  = false,                     // 初始非编辑态
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },		
    {
        .text        = "calibrate",
        .callback    = NULL,                      // 编辑模式下无需回调
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = NULL,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = NULL,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },		
		
	{
        .text        = "Speed_L:?",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = NULL,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_RP_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
	
    {
        .text        = "Speed_R:?",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = NULL,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_RP_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },
    {
        .text        = "Left:?",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = NULL,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_RP_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },	
	
    {
        .text        = "Right:?",
        .callback    = NULL,
        .submenu     = NULL,
        .int16_Value = NULL,
        .float_Value = NULL,
        .item_type   = MENU_ITEM_NORMAL,
        .edit_config = (MenuEditConfig*)&s_RP_edit_config,
        .is_editing  = false,
        .scroll_offset = 0,
        .text_width  = 0,
        .is_scrolling = false
    },		

    // 返回项
    {"[返回]", NULL, NULL, NULL, NULL, MENU_ITEM_BACK},  
    
    // 数组结束标记
    {0}
};

// ==================== More Menu Items ====================
/**
 * @brief More menu items (simple info display)
 */
MyMenuItem MoreItems[] = {
    {"About", NULL, NULL, NULL, MENU_ITEM_NORMAL},                  // 关于信息
    {"Help", NULL, NULL, NULL, MENU_ITEM_NORMAL},                   // 帮助信息
    
    // 返回项
    {"[返回]", NULL, NULL, NULL, NULL, MENU_ITEM_BACK},  
    
    // 数组结束标记
    {0}
};

// ==================== Page Definitions ====================
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
		.last_input_time = 0,
		.input_accel = 1	
};

/**
 * @brief Settings page definition
 * 
 * Subpage configuration要点：
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
		.last_input_time = 0,
		.input_accel = 1	
};

/**
 * @brief Monitor page definition
 * 
 * Includes numeric display functionality
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
		.last_input_time = 0,
		.input_accel = 1
};

/**
 * @brief More page definition
 * 
 * Simple info display page configuration
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
		.last_input_time = 0,
		.input_accel = 1	
};
