/**
 ******************************************************************************
 * @file    debug_config.h
 * @author  Eureka
 * @brief   调试日志配置头文件
 *
 *          提供按模块开关的日志宏，支持条件编译。
 *          所有日志通过 USART_DEBUG 输出，未启用时零开销。
 *
 * @note    使用方法：
 *          1. 在本文件顶部取消注释对应 DEBUG 宏以启用模块日志
 *          2. 在代码中调用 LOG_XXX("msg: %d", val)
 *          3. 确保 bsp_usart.h 中已初始化 USART_DEBUG
 ******************************************************************************
 */

#ifndef DEBUG_CONFIG_H
#define DEBUG_CONFIG_H

#include "usart.h"  // 确保 Serial_Printf 可用

/* ============================================================================ */
/*                     【配置区】按需启用调试模块                                */
/* ============================================================================ */

// ── RTC 实时时钟调试 ───────────────────────────────────────
// #define RTC_DEBUG_INIT   // 启用 RTC 初始化日志


/* ============================================================================ */
/*                     【系统配置】调试串口定义                                  */
/* ============================================================================ */

/**
 * @brief 调试日志输出串口（必须已在 bsp_usart.c 中初始化）
 *
 * 示例：若使用 USART3 作为调试口，则在 bsp_usart.h 中应有：
 *       #define USART_DEBUG USART3
 */
#ifndef USART_DEBUG
    #error "USART_DEBUG is undefined! Please specify the debug serial port in bsp_usart.h or here."
#endif

/* ============================================================================ */
/*                     【日志宏定义】自动条件编译                                */
/* ============================================================================ */

  
// ── RTC 模块 ───────────────────────────────────────────────
#ifdef RTC_DEBUG_INIT
  #define RTC_LOG_INIT(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[RTC] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define RTC_LOG_INIT(fmt, ...) ((void)0)
#endif  
  
    


/* ============================================================================ */
/*                     【扩展指南】新增模块示例                                  */
/* ============================================================================ */
/*
// 1. 在【配置区】添加：
// #define MYMODULE_DEBUG_FOO

// 2. 在此处添加宏：
#ifdef MYMODULE_DEBUG_FOO
  #define MYMODULE_LOG_FOO(fmt, ...) \
      Serial_Printf(USART_DEBUG, "[MYMODULE][FOO] " fmt "\r\n", ##__VA_ARGS__)
#else
  #define MYMODULE_LOG_FOO(fmt, ...) ((void)0)
#endif
*/

#endif /* __DEBUG_CONFIG_H__ */
