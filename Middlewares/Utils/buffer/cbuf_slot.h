#ifndef __CBUF_SLOT_H
#define __CBUF_SLOT_H
#include "stm32f10x.h"                  // Device header
#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

// >>> 用户可在此处注释/取消注释以全局开关断言 <<<
#define CBUF_ENABLE_ASSERTIONS   // ← 注释掉这行即可禁用所有断言，在开发阶段**取消此注释**！！！

// CBUF_ASSERT 行号在 Keil 中可能有偏移（经测试，实际行号比显示行号大24，原因未知），请以表达式内容为准
#ifdef CBUF_ENABLE_ASSERTIONS
    void my_assert_handler(const char* expr, const char* file, int line);
    #define CBUF_ASSERT(expr) do { \
        if (!(expr)) my_assert_handler(#expr, __FILE__, __LINE__); \
    } while(0)
#else
    #define CBUF_ASSERT(expr) ((void)0)
#endif
	

#define CBUF_MAX_INSTANCES 6  // 最大支持6个缓冲区，可根据需求进行调整	
	
typedef struct circular_buf_t circular_buf_t;   // 前置声明：定义类型 circular_buf_t
typedef circular_buf_t* cbuf_handle_t;          // 给 "circular_buf_t 的指针" 起个别名


/// 返回当前已分配的实例数
uint8_t circular_buf_get_instance_count(void);

/// 传入一个存储缓冲区及其大小
/// 返回一个环形缓冲区句柄
cbuf_handle_t circular_buf_init(uint8_t* buffer, size_t size);

/// 将环形缓冲区重置为空状态（head == tail）
void circular_buf_reset(cbuf_handle_t me);

/// 当缓冲区已满时，拒绝写入新数据
/// 成功时返回 0，缓冲区满时返回 -1
int8_t  circular_buf_put(cbuf_handle_t me, uint8_t data);

/// 从缓冲区中取出一个值
/// 成功时返回 0，缓冲区为空时返回 -1
int8_t  circular_buf_get(cbuf_handle_t me, uint8_t* data);

/// 若缓冲区为空，返回 true
bool circular_buf_empty(cbuf_handle_t me);

/// 若缓冲区为满，返回 true
bool circular_buf_full(cbuf_handle_t me);

/// 返回缓冲区的最大容量（元素个数）
size_t circular_buf_capacity(cbuf_handle_t me);

/// 返回缓冲区中当前包含的元素数量
size_t circular_buf_size(cbuf_handle_t me);

size_t circular_buf_peek(cbuf_handle_t me, uint8_t* dst, size_t len);

size_t circular_buf_skip(cbuf_handle_t me, size_t len);


#endif 


//以空行结尾
