/**
 * @file menu_data.h
 * @brief OLED菜单系统数据声明头文件
 * 
 * 本文件声明了菜单系统所需的所有外部数据结构和函数接口。
 * 采用模块化设计，将数据定义与实现分离，便于维护和扩展。
 * 
 * @author Eureka & Lingma
 * @date 2026-02-22
 */
#ifndef MENU_DATA_H
#define MENU_DATA_H

#include "menu_core_types.h"  // 确保类型定义可用
#ifdef __cplusplus
extern "C" {
#endif
	
// 菜单项声明
extern MyMenuItem MainItems[];
extern MyMenuItem SettingsItems[];
extern MyMenuItem MonitorItems[];
extern MyMenuItem MoreItems[];

// 页面声明
extern MyMenuPage MainPage;
extern MyMenuPage SettingsPage;
extern MyMenuPage MonitorPage;
extern MyMenuPage MorePage;

MyMenuID GetMainItemsCount(void);

/** @brief 更新监控数据（在菜单主循环中调用） */
void Update_Monitor_Data(void);

/** @brief 保存 PID 参数到 Flash（用户调整后调用） */
void Save_PID_Params_ToFlash(void);

/** @brief 重置 PID 参数为出厂默认值并保存到 Flash */
void Reset_PID_Params_ToFlash(void);

#ifdef __cplusplus
}
#endif
#endif
