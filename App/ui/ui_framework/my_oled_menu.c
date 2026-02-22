/**
 * @file my_oled_menu.c
 * @brief 轻量级OLED菜单UI框架实现文件
 * 
 * 本文件实现了基于STM32的轻量级OLED菜单系统，支持：
 * - 父子页面导航结构
 * - 按键上下滚动浏览
 * - 长文本水平滚动显示
 * - 稳定的帧率控制机制
 * 
 * @author Eureka & Lingma
 * @date 2026-02-22
 */


#include "my_oled_menu.h"
#include "my_menu_data.h"
#include "menu_core_types.h"
#include "key.h"
#include "bsp_config.h"
#include "bsp_delay.h"
#include "bsp_timer.h"
#include "bsp_input.h"
#include "OLED.h"
#include "OLED_Fonts.h"
#include "system_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
// 全局状态变量
static MyMenuPage* g_current_page = NULL;

/*============================================================================
 *                          静态函数声明
 *============================================================================*/

/*---------------------------- 安全检查相关 ----------------------------*/
static bool IsCurrentPageValid(void);
static void HandleInvalidPage(void);

/*---------------------------- 通用工具函数 ----------------------------*/
static MyMenuID GetMaxVisibleItems(void);
static int16_t CalcStringWidth(int16_t ChineseFont, int16_t ASCIIFont, const char *format, ...);

/*---------------------------- 滚动状态管理 ----------------------------*/
static void InitScrollState(MyMenuItem* item);
static void UpdateVerticalScrollPosition(bool move_down);
static void UpdateHorizontalTextScroll(MyMenuPage* page);
static void SyncSlotWithActiveId(void);

/*---------------------------- 显示处理函数 ----------------------------*/
static void DisplayScrollingText(uint8_t x, uint8_t y, const MyMenuItem* item);
static void DisplayMenuItem(uint8_t x, uint8_t y, MyMenuItem* item, bool is_active);

/*---------------------------- 按键处理函数 ----------------------------*/
static void MyOLED_UI_Enter(void);
static void MyOLED_UI_Back(void);
static void MyOLED_UI_MoveUp(void);
static void MyOLED_UI_MoveDown(void);

/*============================================================================
 *                          安全检查相关函数
 *============================================================================*/

/**
 * @brief 检查当前页面是否有效
 * 
 * 验证当前页面指针、菜单项数组和项目数量的有效性
 * 
 * @return bool true表示页面有效，false表示页面无效
 * @note 用于所有页面操作前的安全检查
 */
static bool IsCurrentPageValid(void) {
    if (g_current_page == NULL || 
        g_current_page->items == NULL || 
        g_current_page->ItemNum == 0) {
        return false;
    }
    return true;
}

/**
 * @brief 处理无效页面状态
 * 
 * 当检测到页面无效时，显示相应的错误信息
 * 
 * @note 根据具体错误类型显示"NO PAGE"或"EMPTY"提示
 */
static void HandleInvalidPage(void) {
    OLED_Clear();
    if (g_current_page == NULL || g_current_page->items == NULL) {
        OLED_ShowString(0, 0, "NO PAGE", OLED_8X16_HALF);
    } else {
        OLED_ShowString(0, 0, "EMPTY", OLED_8X16_HALF);
    }
    OLED_Update();
}


/*============================================================================
 *                          通用工具函数
 *============================================================================*/

/**
 * @brief 获取最大可见菜单项数量
 * 
 * 根据屏幕高度和字体大小计算可同时显示的菜单项数量
 * 
 * @return MyMenuID 最大可见项数
 * @note 基于硬件配置参数MAX_SAFE_VISIBLE_ITEMS
 */
static MyMenuID GetMaxVisibleItems(void) {
    return MAX_SAFE_VISIBLE_ITEMS;
}

/**
 * @brief 计算字符串显示宽度
 * 
 * 根据中英文字体规格计算字符串的实际显示像素宽度
 * 支持可变参数格式化字符串
 * 
 * @param ChineseFont 中文字体宽度（像素）
 * @param ASCIIFont ASCII字体宽度（像素）
 * @param format 格式化字符串
 * @param ... 可变参数列表
 * @return int16_t 字符串总宽度（像素）
 * @note 自动识别UTF-8编码的中英文字符
 */
static int16_t CalcStringWidth(int16_t ChineseFont, int16_t ASCIIFont, const char *format, ...) {
    int16_t StringLength = 0;
    char String[MAX_STRING_LENGTH];

    va_list args;
    va_start(args, format);
    vsnprintf(String, sizeof(String), format, args);
    va_end(args);

    char *ptr = String;
    if(OLED_CHN_CHAR_WIDTH == 2){
        while (*ptr != '\0') {
            if ((unsigned char)*ptr & 0x80) {
                StringLength += ChineseFont;
                ptr += 2;
            } else {
                StringLength += ASCIIFont;
                ptr++;
            }
        }
    }
    else if(OLED_CHN_CHAR_WIDTH == 3){
        while (*ptr != '\0') {
            uint8_t c = (uint8_t)*ptr;
            if ((c & 0xE0) == 0xE0) {
                StringLength += ChineseFont;
                ptr += 3;
            } else if ((c & 0xC0) == 0xC0) {
                ptr += 2;
            } else {
                StringLength += ASCIIFont;
                ptr++;
            }
        }   
    }
    return StringLength;
}


/*============================================================================
 *                          滚动状态管理函数
 *============================================================================*/

/**
 * @brief 初始化菜单项滚动状态
 * 
 * 根据文本宽度判断是否需要启用水平滚动，并设置初始状态
 * 
 * @param item 指向要初始化的菜单项
 */
static void InitScrollState(MyMenuItem* item) {
    if (item == NULL) return;
    
    item->text_width = CalcStringWidth(OLED_16X16_FULL, OLED_8X16_HALF, item->text, *item->u16_Value);
    
    int16_t available_width = OLED_WIDTH - START_POINT_OF_TEXT_DISPLAY;
    
    if (item->text_width > available_width) {
        item->is_scrolling = true;
        item->scroll_offset = 0;
    } else {
        item->is_scrolling = false;
        item->scroll_offset = 0;
    }
}

/**
 * @brief 更新垂直滚动位置
 * 
 * 处理菜单项的上下导航逻辑，包括光标移动和可视区域滚动
 * 
 * @param move_down true表示向下移动，false表示向上移动
 * @note 控制slot和visible_start两个核心状态变量
 */
static void UpdateVerticalScrollPosition(bool move_down) {
    if (!IsCurrentPageValid()) return;
    
    MyMenuID max_visible = GetMaxVisibleItems();
    MyMenuID max_slot = max_visible - 1;
    
    if (move_down) {
        if (g_current_page->slot < max_slot) {
            g_current_page->slot++;
        } else {
            MyMenuID max_visible_start = (g_current_page->ItemNum > max_visible) ? 
                                       (g_current_page->ItemNum - max_visible) : 0;
            if (g_current_page->visible_start < max_visible_start) {
                g_current_page->visible_start++;
            }
        }
    } else {
        if (g_current_page->slot > 0) {
            g_current_page->slot--;
        } else {
            if (g_current_page->visible_start > 0) {
                g_current_page->visible_start--;
            }
        }
    }
}

/**
 * @brief 更新水平文本滚动动画
 * 
 * 处理长文本的水平滚动显示，实现无缝循环效果
 * 
 * @param page 指向当前页面
 * @note 独立于垂直滚动，使用单独的时间基准控制
 */
static void UpdateHorizontalTextScroll(MyMenuPage* page) {
    if (page == NULL) {
        return;
    }
    
    uint32_t current_time = SysTick_Get();
    
    if (page->last_scroll_time == 0) {
        page->last_scroll_time = current_time;
    }
    
    if ((current_time - page->last_scroll_time) >= page->scroll_delay) {
        page->last_scroll_time = current_time;
        
        MyMenuID start_idx = page->visible_start;
        MyMenuID end_idx = start_idx + GetMaxVisibleItems();
        if (end_idx > page->ItemNum) {
            end_idx = page->ItemNum;
        }
        
        for (MyMenuID i = start_idx; i < end_idx; i++) {
            MyMenuItem* item = &page->items[i];
            if (item->is_scrolling) {
                item->scroll_offset--;
                
                if (item->scroll_offset < -(int16_t)item->text_width) {
                    item->scroll_offset = OLED_WIDTH - START_POINT_OF_TEXT_DISPLAY;
                }
            }
        }
    }
}

/**
 * @brief 同步槽位与活动ID
 * 
 * 确保slot（可视槽位）与active_id（活动项索引）的位置关系正确
 * 
 * @note 在每次导航操作后调用，维护UI状态一致性
 */
static void SyncSlotWithActiveId(void) {
    if (!IsCurrentPageValid()) return;
    
    if (g_current_page->active_id >= g_current_page->ItemNum) {
        g_current_page->active_id = g_current_page->ItemNum - 1;
    }
    
    MyMenuID offset = g_current_page->active_id - g_current_page->visible_start;
    MyMenuID actual_max_visible = GetMaxVisibleItems();
    
    if (offset < actual_max_visible) {
        g_current_page->slot = offset;
    } else {
        g_current_page->visible_start = g_current_page->active_id - (actual_max_visible - 1);
        g_current_page->slot = actual_max_visible - 1;
    }
    
    if (g_current_page->slot >= actual_max_visible) {
        g_current_page->slot = actual_max_visible - 1;
    }
}


/*============================================================================
 *                          显示处理函数
 *============================================================================*/

/**
 * @brief 显示滚动文本
 * 
 * 处理长文本的水平滚动显示，确保在指定区域内正确渲染
 * 
 * @param x 显示起始X坐标
 * @param y 显示Y坐标
 * @param item 指向要显示的菜单项
 * @note 自动处理文本边界和前缀避让
 */
static void DisplayScrollingText(uint8_t x, uint8_t y, const MyMenuItem* item) {
    if (item == NULL) {
        return;
    }
    
    int16_t display_x = x + item->scroll_offset;
    
    if (display_x <= OLED_WIDTH) {
        OLED_PrintfMix(display_x, y, OLED_16X16_FULL, OLED_8X16_HALF, item->text, *item->u16_Value);
    }
}

/**
 * @brief 显示单个菜单项
 * 
 * 统一处理菜单项的显示逻辑，包括前缀、滚动状态和数值显示
 * 
 * @param x 显示起始X坐标
 * @param y 显示Y坐标
 * @param item 指向要显示的菜单项
 * @param is_active 是否为当前活动项
 * @note 集成了动态文本宽度检测和滚动状态管理
 */
static void DisplayMenuItem(uint8_t x, uint8_t y, MyMenuItem* item, bool is_active) {
    char prefix[4] = {0};
    
    uint8_t text_x = x + START_POINT_OF_TEXT_DISPLAY;
    
    bool is_dynamic_content = (item->u16_Value != NULL);
    
    if (is_dynamic_content) {
        int16_t new_width = CalcStringWidth(OLED_16X16_FULL, OLED_8X16_HALF, item->text, *item->u16_Value);
        
        if (abs(new_width - item->text_width) >= 8) {
            item->text_width = new_width;
            
            bool should_scroll = (item->text_width > (OLED_WIDTH - START_POINT_OF_TEXT_DISPLAY));
            if (should_scroll != item->is_scrolling) {
                item->is_scrolling = should_scroll;
                if (!should_scroll) {
                    item->scroll_offset = 0;
                }
            }
        }
    }
    
    if (item->is_scrolling) {
        DisplayScrollingText(text_x, y, item);
    } else {
        if (item->u16_Value != NULL) {
            OLED_PrintfMix(text_x, y, OLED_16X16_FULL, OLED_8X16_HALF, item->text, *item->u16_Value);
        } else {
            OLED_PrintfMix(text_x, y, OLED_16X16_FULL, OLED_8X16_HALF, item->text);
        }
    }
    
    prefix[0] = is_active ? '>' : ' ';
    
    switch(item->item_type) {
        case MENU_ITEM_BACK:
            prefix[1] = '~';
            break;
        default:
            if (item->submenu != NULL) {
                prefix[1] = '>';
            } else if (item->callback != NULL) {
                prefix[1] = '+';
            } else {
                prefix[1] = '-';
            }
            break;
    }
    
    OLED_ShowString(x, y, prefix, OLED_8X16_HALF);
}

/*============================================================================
 *                          按键处理函数
 *============================================================================*/

/**
 * @brief 处理Enter按键事件
 * 
 * 执行当前活动项的确认操作，可能包括页面跳转或回调执行
 * 
 * @note 优先处理特殊类型项（如返回项）
 */
static void MyOLED_UI_Enter(void) {
    if (!IsCurrentPageValid()) return;
    
    const MyMenuItem* item = &g_current_page->items[g_current_page->active_id];
    
    switch(item->item_type) {
        case MENU_ITEM_BACK:
            if (g_current_page->parent != NULL) {
                g_current_page = g_current_page->parent;
            }
            return;
            
        default:
            if (item->submenu != NULL) {
                g_current_page = item->submenu;
                g_current_page->active_id = 0;
                g_current_page->visible_start = 0;
                g_current_page->slot = 0;
            } else if (item->callback != NULL) {
                item->callback();
            }
            break;
    }
}

/**
 * @brief 处理Back按键事件
 * 
 * 返回到父页面（如果存在）
 * 
 * @note 简单的页面栈回退操作
 */
static void MyOLED_UI_Back(void) {
    if (g_current_page != NULL && g_current_page->parent != NULL) {
        g_current_page = g_current_page->parent;
    }
}

/**
 * @brief 处理向上导航
 * 
 * 将活动项向上移动，必要时滚动可视区域
 * 
 * @note 调用UpdateVerticalScrollPosition进行实际位置更新
 */
static void MyOLED_UI_MoveUp(void) {
    if (!IsCurrentPageValid()) return;
    
    if (g_current_page->active_id == 0) return;
    
    g_current_page->active_id--;
    UpdateVerticalScrollPosition(false);
}

/**
 * @brief 处理向下导航
 * 
 * 将活动项向下移动，必要时滚动可视区域
 * 
 * @note 调用UpdateVerticalScrollPosition进行实际位置更新
 */
static void MyOLED_UI_MoveDown(void) {
    if (!IsCurrentPageValid()) return;
    
    if (g_current_page->active_id >= g_current_page->ItemNum - 1) return;
    
    g_current_page->active_id++;
    UpdateVerticalScrollPosition(true);
}

/*============================================================================
 *                          对外接口函数
 *============================================================================*/

/**
 * @brief 初始化OLED菜单系统
 * 
 * 设置初始页面并初始化必要的硬件组件
 * 
 * @param page 指向初始页面
 * @note 必须在系统启动时调用一次
 */
void MyOLED_UI_Init(MyMenuPage* page) {
    Delay_Init();                     // 必须最先初始化（用于后续延时）
    OLED_Init();//OLED屏初始化，与数据显示有关
    Timer2_Init();//定时2初始化，与任务调度有关	
    Key_Init();
    
    if (page == NULL) {
        OLED_Clear();
        OLED_ShowString(0, 0, "PAGE NULL!", OLED_8X16_HALF);
        OLED_Update();
        while (1);
    }
    
    g_current_page = page;
}

/**
 * @brief 主循环函数
 * 
 * UI系统的核心循环，处理按键输入、更新状态并刷新显示
 * 包含帧率控制机制确保稳定的刷新频率
 * 
 * @note 应该在main函数中循环调用
 */
void MyOLED_UI_MainLoop(void) {
    static KeyEventType s_key_events[MAX_KEYS_NUM] = {KEY_EVENT_NONE};
    Key_GetEvent(s_key_events, MAX_KEYS_NUM);
    
    // 处理按键事件
    if (s_key_events[0] == KEY_EVENT_CLICK) {
        MyOLED_UI_Enter();
    }
    if (s_key_events[1] == KEY_EVENT_CLICK) {
        MyOLED_UI_Back();
    }
    if (s_key_events[2] == KEY_EVENT_CLICK || s_key_events[2] == KEY_EVENT_LONG_PRESS) {
        MyOLED_UI_MoveUp();
    }
    if (s_key_events[3] == KEY_EVENT_CLICK || s_key_events[3] == KEY_EVENT_LONG_PRESS) {
        MyOLED_UI_MoveDown();
    }

    // 统一的安全检查和错误处理
    if (!IsCurrentPageValid()) {
        HandleInvalidPage();
        return;
    }

    // 同步槽位与活动ID
    SyncSlotWithActiveId();
    
    // 独立更新水平文本滚动动画
    UpdateHorizontalTextScroll(g_current_page);
    
    // 确保visible_start有效
    MyMenuID actual_displayable = GetMaxVisibleItems();
    MyMenuID max_visible_start = (g_current_page->ItemNum > actual_displayable) ? 
                               (g_current_page->ItemNum - actual_displayable) : 0;
    if (g_current_page->visible_start > max_visible_start) {
        g_current_page->visible_start = max_visible_start;
    }

    OLED_Clear();

    // 显示菜单项
    MyMenuID start_idx = g_current_page->visible_start;
    MyMenuID end_idx = start_idx + actual_displayable;
    if (end_idx > g_current_page->ItemNum) {
        end_idx = g_current_page->ItemNum;
    }
    
    MyMenuID display_index = 0;
    for (MyMenuID i = start_idx; i < end_idx; i++) {
        uint8_t line_y = display_index * LINE_HEIGHT;
        
        if (line_y > (OLED_HEIGHT - LINE_HEIGHT)) {
            break;
        }
        
        display_index++;
        
        MyMenuItem* item = &g_current_page->items[i];
        if (item->text == NULL) continue;

        if (item->text_width == 0) {
            InitScrollState(item);
        }

        bool is_active = (i == g_current_page->active_id);
        DisplayMenuItem(0, line_y, item, is_active);
    }

    OLED_Update();
    
    // 添加稳定的帧率控制
    static uint32_t last_frame_time = 0;
    uint32_t current_time = SysTick_Get();
    
    uint32_t frame_interval = SET_FRAME_INTERVAL;
    if ((current_time - last_frame_time) >= frame_interval) {
        last_frame_time = current_time;
    } else {
        uint32_t delay_needed = frame_interval - (current_time - last_frame_time);
        if (delay_needed > 0 && delay_needed <= frame_interval) {
            Delay_ms(delay_needed);
        }
        last_frame_time = SysTick_Get();
    }
}
