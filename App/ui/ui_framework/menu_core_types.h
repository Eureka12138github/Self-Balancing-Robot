/**
 * @file menu_core_types.h
 * @brief OLED菜单系统核心数据类型定义文件
 * 
 * 本文件定义了菜单系统所需的所有核心数据结构和配置参数。
 * 包括硬件参数配置、枚举类型定义和主要结构体声明。
 * 采用前向声明解决结构体循环依赖问题。
 * 
 * @author Eureka & Lingma
 * @date 2026-02-22
 */

#ifndef MENU_CORE_TYPES_H
#define MENU_CORE_TYPES_H

#include <stdint.h>
#include <stdbool.h>

/*============================================================================
 *                          核心类型定义
 *============================================================================*/

/** @brief 菜单ID类型定义 */
typedef uint16_t MyMenuID;

// 前向声明解决循环依赖问题
struct MyMenuPage;
struct MyMenuItem;

/*============================================================================
 *                          枚举类型定义
 *============================================================================*/

/**
 * @brief 菜单项类型枚举
 * 
 * 定义菜单项的不同功能类型，用于区分普通项和特殊功能项
 */
typedef enum {
    MENU_ITEM_NORMAL = 0,     /**< 普通菜单项 */
    MENU_ITEM_BACK = 1,       /**< 返回上级菜单项 */
} MenuItemType;

/*============================================================================
 *                          结构体定义
 *============================================================================*/

/**
 * @brief 菜单项结构体
 * 
 * 定义单个菜单项的所有属性和功能配置
 */
typedef struct MyMenuItem {
    const char* text;                    /**< 显示文本内容(支持格式化字符串) */
    void (*callback)(void);              /**< 回调函数指针(确认键执行) */
    struct MyMenuPage* submenu;          /**< 子菜单页面指针(用于页面跳转，优先级高于回调函数) */
    uint16_t* u16_Value;                 /**< 关联的16位数值变量指针 */
    MenuItemType item_type;              /**< 菜单项类型(普通/返回等) */
    
    // 滚动相关字段
    int16_t scroll_offset;               /**< 水平滚动偏移量(像素) */
    uint16_t text_width;                 /**< 文本宽度缓存(像素) */
    bool is_scrolling;                   /**< 滚动状态标志 */
} MyMenuItem;

/**
 * @brief 菜单页面结构体
 * 
 * 定义一个完整菜单页面的所有状态和配置信息
 */
typedef struct MyMenuPage {
    struct MyMenuPage* parent;           /**< 父页面指针(用于返回操作) */
    MyMenuItem* items;                   /**< 菜单项数组指针 */
    MyMenuID active_id;                  /**< 当前活动项索引 */
    MyMenuID visible_start;              /**< 可视区域起始项索引 */
    MyMenuID max_visible;                /**< 最大可见项数 */
    MyMenuID slot;                       /**< 当前光标槽位(0到max_visible-1) */
    MyMenuID ItemNum;                    /**< 菜单项总数 */
    
    // 页面级滚动控制
    uint32_t last_scroll_time;           /**< 上次滚动更新时间戳 */
    uint16_t scroll_delay;               /**< 滚动动画更新间隔(毫秒) */
} MyMenuPage;

#endif // MENU_CORE_TYPES_H
