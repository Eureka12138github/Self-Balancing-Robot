/**
 * @file menu.c
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


#include "menu.h"
#include "menu_data.h"
#include "menu_core_types.h"
#include "key.h"
// #include "rotary_encoder.h"  // 旋转编码器已删除
#include "bsp_config.h"
#include "delay.h"
#include "timer.h"
#include "OLED.h"                    
#include "OLED_Fonts.h"
#include "system_config.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>  // 提供 strchr, strncpy 函数
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
 * @brief 统一处理数值编辑逻辑（支持 Key 和 Encoder，支持 int16 和 float）
 * 
 * @param active_item   当前激活的菜单项
 * @param direction     方向：1 为增加，-1 为减少
 * @param is_fast_input 是否处于“快速输入”状态 
 *                      (对于 Encoder: 只要转动即为 true; 
 *                       对于 Key: 仅在长按时为 true)
 */
static void MyOLED_Process_Value_Edit(MyMenuItem* active_item, int8_t direction, bool is_fast_input) {
    if (!active_item || !active_item->edit_config) return;

    uint32_t now = SysTick_Get();

    // 1. 初始化/更新加速状态 (使用通用命名)
    if (g_current_page->last_input_time == 0) {
        g_current_page->last_input_time = now;
        g_current_page->input_accel = 1;
    }

    uint32_t dt = now - g_current_page->last_input_time;
    
    // 加速逻辑：
    // 条件：处于快速输入模式 (长按或连续转动) AND 时间间隔极短 (<100ms)
    if (is_fast_input && dt < 100 && dt > 0) {
        g_current_page->input_accel++;
        if (g_current_page->input_accel > 4) {
            g_current_page->input_accel = 4; // 最大加速等级
        }
    } else {
        // 非快速输入 (短按) 或 间隔过长 -> 重置为慢速
        g_current_page->input_accel = 1;
    }
    g_current_page->last_input_time = now;

    // 定义加速倍率表 (通用)
    // 索引 0(初始):1, 1(慢):1, 2(中):5, 3(快):10, 4(极速):15
    const uint8_t speed_boost_table[5] = {1, 1, 5, 10, 15};
    
    int32_t base_step = active_item->edit_config->step;
    if (base_step <= 0) base_step = 1;

    // ---------------------------------------------------------
    // 分支 A: 处理 int16 类型
    // ---------------------------------------------------------
    if (active_item->int16_Value != NULL) {
        int32_t boost_factor = speed_boost_table[g_current_page->input_accel];
        int32_t actual_step = base_step * boost_factor;
        
        int32_t current_val = (int32_t)(*active_item->int16_Value);
        int32_t new_val = current_val + (direction * actual_step);

        // 边界限制
        if (new_val < active_item->edit_config->min) {
            new_val = active_item->edit_config->min;
        } else if (new_val > active_item->edit_config->max) {
            new_val = active_item->edit_config->max;
        }

        *active_item->int16_Value = (uint16_t)new_val;
    }
    // ---------------------------------------------------------
    // 分支 B: 处理 float 类型
    // ---------------------------------------------------------
    else if (active_item->float_Value != NULL) {
        // 浮点步长计算：base_step * 系数 * 加速倍率
        // 系数 0.001f 可根据实际需求调整，或从 config 读取
        float base_step_f = (float)base_step * 0.001f; 
        float boost_factor_f = (float)speed_boost_table[g_current_page->input_accel];
        float actual_step_f = base_step_f * boost_factor_f;
        
        float current_val = *active_item->float_Value;
        float new_val = current_val + (direction * actual_step_f);

        // 边界限制 (将 config 的 int min/max 转为 float 比较)
        float f_min = (float)active_item->edit_config->min;
        float f_max = (float)active_item->edit_config->max;

        if (new_val < f_min) {
            new_val = f_min;
        } else if (new_val > f_max) {
            new_val = f_max;
        }

        *active_item->float_Value = new_val;
    }
}

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
 * @param ASCIIFont ASCII 字体宽度（像素）
 * @param format 格式化字符串
 * @param ... 可变参数列表
 * @return int16_t 字符串总宽度（像素）
 * @note 自动识别 UTF-8 编码的中英文字符
 * @warning Newlib-Nano 限制：PlatformIO + CMSIS 环境下 vsnprintf 不支持 %f 浮点格式；
 *         计算含浮点数的字符串宽度时，需先手动格式化为整数形式再传入
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
    
    // 根据数值类型选择格式化方式计算文本宽度
    if(item->float_Value != NULL) {
        // float 类型优先处理（PID 参数等使用）
        // ⚠️ 注意：此处 CalcStringWidth 内部使用 vsnprintf，在 Newlib-Nano 下不支持 %f
        // 实际项目中已改为手动格式化浮点数为整数形式后再调用（见 DisplayScrollingText 函数）
        item->text_width = CalcStringWidth(OLED_16X16_FULL, OLED_8X16_HALF, item->text, *item->float_Value);
    }
    else if(item->int16_Value != NULL) {
        // int16 类型
        item->text_width = CalcStringWidth(OLED_16X16_FULL, OLED_8X16_HALF, item->text, *item->int16_Value);
    }
    else {
        // 纯文本，无动态数值
        item->text_width = CalcStringWidth(OLED_16X16_FULL, OLED_8X16_HALF, item->text);	
    }

    
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
 * 处理长文本的水平滚动显示，实现无缝循环效果g
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
                
                if (item->scroll_offset < -item->text_width) {
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

static void DisplayScrollingText(uint8_t x, uint8_t y, const MyMenuItem* item) {
    if (item == NULL || item->text_width == 0) return;
    
    int16_t display_x = x + item->scroll_offset;
    int16_t text_right_edge = display_x + item->text_width;
    
    // 👇 关键：仅当文本与 [0, OLED_WIDTH) 区域有重叠时才绘制
    if (display_x < OLED_WIDTH && text_right_edge > 0) {
		//新实现：长字符串流畅显示
		// float 类型优先处理（PID 参数等使用）
        if(item->float_Value != NULL){
			// ========== 手动格式化浮点数（绕过 vsprintf 限制）==========
			float test_val = *item->float_Value;
			char buffer[32];
			int int_part = (int)test_val;
			int frac_part = (int)((test_val - int_part) * 1000);  // 取小数部分 3 位
			if (frac_part < 0) frac_part = -frac_part;  // 取绝对值
			
			// ⚠️ 关键：找到 item->text 中的冒号并提取前缀
			const char *colon_pos = strchr(item->text, ':');
			if (colon_pos != NULL) {
				size_t prefix_len = colon_pos - item->text + 1;
				char prefix[16];
				strncpy(prefix, item->text, prefix_len);
				prefix[prefix_len] = '\0';
				snprintf(buffer, sizeof(buffer), "%s%+d.%03d", 
				        prefix, int_part, frac_part);
			} else {
				snprintf(buffer, sizeof(buffer), "%s:%+d.%03d", 
				        item->text, int_part, frac_part);
			}
			
			// ⚠️ 关键：使用实际格式化后的字符串重新计算宽度，确保滚动正确
			int16_t actual_width = CalcStringWidth(OLED_16X16_FULL, OLED_8X16_HALF, buffer);
			OLED_PrintfMixArea(display_x, y, actual_width, FONT_HEIGHT, display_x, y, 
			                   OLED_16X16_FULL, OLED_8X16_HALF, buffer);
		}
		else if(item->int16_Value != NULL) {
			OLED_PrintfMixArea(display_x, y, item->text_width, FONT_HEIGHT, display_x, y, 
			                   OLED_16X16_FULL, OLED_8X16_HALF, item->text, *item->int16_Value);
		}else {
			OLED_ShowMixStringArea(display_x, y, item->text_width, FONT_HEIGHT, display_x, y, 
			                       item->text, OLED_16X16_FULL, OLED_8X16_HALF);
		}	
    }
    // 否则：完全在左侧（text_right_edge <= 0）或完全在右侧（display_x >= OLED_WIDTH），跳过
}   

/**
 * @brief 显示单个菜单项
 * 
 * 统一处理菜单项的显示逻辑，包括前缀、滚动状态和数值显示
 * 
 * @param x 显示起始 X 坐标
 * @param y 显示 Y 坐标
 * @param item 指向要显示的菜单项
 * @param is_active 是否为当前活动项
 * @note 集成了动态文本宽度检测和滚动状态管理
 */
static void DisplayMenuItem(uint8_t x, uint8_t y, MyMenuItem* item, bool is_active) {
    char prefix[4] = {0};
    bool is_dynamic_content = false;	
    uint8_t text_x = x + START_POINT_OF_TEXT_DISPLAY;
	
    // 判断是否为动态内容
    if(item->float_Value != NULL || item->int16_Value != NULL) {
		is_dynamic_content = true;
	}
    
    if (is_dynamic_content) {
		int16_t new_width;
		// float 类型优先处理（PID 参数等使用）
		if(item->float_Value != NULL) {
			new_width = CalcStringWidth(OLED_16X16_FULL, OLED_8X16_HALF, item->text, *item->float_Value);
		}
		else if(item->int16_Value != NULL) {
			new_width = CalcStringWidth(OLED_16X16_FULL, OLED_8X16_HALF, item->text, *item->int16_Value);		
		}else {
			// 理论上不会到这里，防御性编程
			new_width = CalcStringWidth(OLED_16X16_FULL, OLED_8X16_HALF, item->text);
		}

        
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
        // 根据数值类型选择显示方式
        if (item->float_Value != NULL) {
            // float 类型（PID 参数使用）
            // ========== 手动格式化浮点数（绕过 vsprintf 限制）==========
            float test_val = *item->float_Value;
            char buffer[32];
            int int_part = (int)test_val;
            int frac_part = (int)((test_val - int_part) * 1000);
            if (frac_part < 0) frac_part = -frac_part;
            
            const char *colon_pos = strchr(item->text, ':');
            if (colon_pos != NULL) {
                size_t prefix_len = colon_pos - item->text + 1;
                char prefix[16];
                strncpy(prefix, item->text, prefix_len);
                prefix[prefix_len] = '\0';
                snprintf(buffer, sizeof(buffer), "%s%+d.%03d", 
                        prefix, int_part, frac_part);
            } else {
                snprintf(buffer, sizeof(buffer), "%s:%+d.%03d", 
                        item->text, int_part, frac_part);
            }
            
            // ✅ 非滚动模式：直接显示完整字符串
            OLED_ShowMixString(text_x, y, buffer, OLED_16X16_FULL, OLED_8X16_HALF);
            
        } else if (item->int16_Value != NULL) {
            // int16 类型
            OLED_PrintfMix(text_x, y, OLED_16X16_FULL, OLED_8X16_HALF, item->text, *item->int16_Value);
        } else {
            // 纯文本
            OLED_ShowMixString(text_x, y, item->text, OLED_16X16_FULL, OLED_8X16_HALF);
        }
    }

    // 根据活动状态和编辑状态决定主前缀
    if (is_active) {
        if (item->is_editing) {
            prefix[0] = '=';
        } else {
            prefix[0] = '>';
        }
    } else {
        prefix[0] = ' ';
    }
    
    switch(item->item_type) {
        case MENU_ITEM_BACK:
            prefix[1] = '~';
            break;
        default:
            if (item->submenu != NULL) {
                prefix[1] = '>';
            } else if (item->callback != NULL) {
                prefix[1] = '+';
            } else if(item->edit_config != NULL){
                prefix[1] = '=';
            }
			else {
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
 * 执行当前活动项的确认操作，可能包括页面跳转、回调执行或切换编辑模式
 * 
 * @note 优先级：返回项 > 子菜单 > 编辑模式 > 回调函数
 */
static void MyOLED_UI_Enter(void) {
    if (!IsCurrentPageValid()) return;
    
    MyMenuItem* item = &g_current_page->items[g_current_page->active_id]; 
    
    // 返回项处理（最高优先级）
    if (item->item_type == MENU_ITEM_BACK) {
        if (g_current_page->parent != NULL) {
            g_current_page = g_current_page->parent;
        }
        return;
    }
    
    // 子菜单跳转（次高优先级）
    if (item->submenu != NULL) {
        g_current_page = item->submenu;
        g_current_page->active_id = 0;
        g_current_page->visible_start = 0;
        g_current_page->slot = 0;
        return;
    }
    
    // 可编辑项处理
    if (item->edit_config != NULL) {
        item->is_editing = !item->is_editing;  // 切换编辑状态
        return;
    }
    
    // 普通回调（最后）
    if (item->callback != NULL) {
        item->callback();
    }
}

static void MyOLED_UI_Back(void) {
    if (!IsCurrentPageValid()) return;
    
    // 如果当前活动项正在编辑，先退出编辑模式，不返回上级
    MyMenuItem* item = &g_current_page->items[g_current_page->active_id];
    if (item->is_editing) {
        item->is_editing = false;
        return;
    }
    
    // 否则正常返回上级
    if (g_current_page->parent != NULL) {
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
    Timer1_Init();//定时1初始化，与任务调度有关	
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
 * 采用非阻塞帧率控制机制：仅在达到设定帧间隔时才执行耗时的屏幕渲染，
 * 其余时间快速返回，确保输入响应实时性。
 * 
 * @note 应该在 main 函数中循环调用，调用频率应高于 SET_FRAME_INTERVAL（如 1~5ms 一次）
 */
void MyOLED_UI_MainLoop(void) {
	

	
    // 【始终执行】采集按键事件
    static KeyEventType s_key_events[MAX_KEYS_NUM] = {KEY_EVENT_NONE};
    Key_GetEvent(s_key_events, MAX_KEYS_NUM);


    // 【始终执行】处理按键事件
    if (s_key_events[0] == KEY_EVENT_CLICK) {
        MyOLED_UI_Enter();
    }
    if (s_key_events[1] == KEY_EVENT_CLICK) {
        MyOLED_UI_Back();
    }
		// 处理 key3 (索引2): 减小 / 向上
		if (s_key_events[2] != KEY_EVENT_NONE) {
				MyMenuItem* active_item = &g_current_page->items[g_current_page->active_id];

				// 检查是否处于编辑模式 且 有有效的数值指针 (int 或 float)
				if (active_item->is_editing && active_item->edit_config != NULL && 
						(active_item->int16_Value != NULL || active_item->float_Value != NULL)) {
						
						bool is_long_press = (s_key_events[2] == KEY_EVENT_LONG_PRESS);
						
						// 调用统一函数：方向 -1 (减小)
						MyOLED_Process_Value_Edit(active_item, -1, is_long_press);
				} 
				else {
						// 非编辑模式：导航
						MyOLED_UI_MoveUp();
						// 重置加速状态
						g_current_page->input_accel = 1;
						g_current_page->last_input_time = 0;
				}
		}

		// 处理 key4 (索引3): 增大 / 向下
		if (s_key_events[3] != KEY_EVENT_NONE) {
				MyMenuItem* active_item = &g_current_page->items[g_current_page->active_id];

				// 检查是否处于编辑模式 且 有有效的数值指针
				if (active_item->is_editing && active_item->edit_config != NULL && 
						(active_item->int16_Value != NULL || active_item->float_Value != NULL)) {
						
						bool is_long_press = (s_key_events[3] == KEY_EVENT_LONG_PRESS);
						
						// 调用统一函数：方向 +1 (增大)
						MyOLED_Process_Value_Edit(active_item, 1, is_long_press);
				} 
				else {
						// 非编辑模式：导航
						MyOLED_UI_MoveDown();
						// 重置加速状态
						g_current_page->input_accel = 1;
						g_current_page->last_input_time = 0;
				}
		}


    // 【始终执行】统一的安全检查和错误处理
    if (!IsCurrentPageValid()) {
        HandleInvalidPage();
        return;
    }		


    // 【始终执行】同步槽位与活动ID
    SyncSlotWithActiveId();
    
    // 【始终执行】更新水平文本滚动动画（基于时间，轻量）
    UpdateHorizontalTextScroll(g_current_page);
    
    // 【始终执行】确保 visible_start 有效
    MyMenuID actual_displayable = GetMaxVisibleItems();
    MyMenuID max_visible_start = (g_current_page->ItemNum > actual_displayable) ? 
                               (g_current_page->ItemNum - actual_displayable) : 0;
    if (g_current_page->visible_start > max_visible_start) {
        g_current_page->visible_start = max_visible_start;
    }

    // =============================================
    // ✅ 非阻塞帧率控制：仅当达到帧间隔时才渲染
    // =============================================
    static uint32_t last_render_time = 0;
    uint32_t current_time = SysTick_Get();
    
    if ((current_time - last_render_time) < SET_FRAME_INTERVAL) {
        // ⏱️ 未到刷新时间，跳过渲染以避免阻塞
        return;
    }
    last_render_time = current_time;

    // 【仅在此处执行】
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

    OLED_Update(); // 实际刷屏
}

