/**
 * @brief 测试程序：验证 MyMenuItem 结构体成员偏移量
 * 
 * 通过串口打印结构体各成员的偏移量，验证内存对齐是否正确
 */

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include "menu_core_types.h"

void PrintStructOffsets(void) {
    printf("=== MyMenuItem Structure Layout ===\r\n");
    printf("Size of MyMenuItem: %d bytes\r\n", sizeof(MyMenuItem));
    printf("\r\n");
    
    printf("Offset of each member:\r\n");
    printf("  text:         +%d (0x%02X)\r\n", offsetof(MyMenuItem, text), offsetof(MyMenuItem, text));
    printf("  callback:     +%d (0x%02X)\r\n", offsetof(MyMenuItem, callback), offsetof(MyMenuItem, callback));
    printf("  submenu:      +%d (0x%02X)\r\n", offsetof(MyMenuItem, submenu), offsetof(MyMenuItem, submenu));
    printf("  int16_Value:  +%d (0x%02X)\r\n", offsetof(MyMenuItem, int16_Value), offsetof(MyMenuItem, int16_Value));
    printf("  float_Value:  +%d (0x%02X)\r\n", offsetof(MyMenuItem, float_Value), offsetof(MyMenuItem, float_Value));
    printf("  item_type:    +%d (0x%02X)\r\n", offsetof(MyMenuItem, item_type), offsetof(MyMenuItem, item_type));
    printf("  edit_config:  +%d (0x%02X)\r\n", offsetof(MyMenuItem, edit_config), offsetof(MyMenuItem, edit_config));
    printf("  is_editing:   +%d (0x%02X)\r\n", offsetof(MyMenuItem, is_editing), offsetof(MyMenuItem, is_editing));
    printf("  scroll_offset:+%d (0x%02X)\r\n", offsetof(MyMenuItem, scroll_offset), offsetof(MyMenuItem, scroll_offset));
    printf("  text_width:   +%d (0x%02X)\r\n", offsetof(MyMenuItem, text_width), offsetof(MyMenuItem, text_width));
    printf("  is_scrolling: +%d (0x%02X)\r\n", offsetof(MyMenuItem, is_scrolling), offsetof(MyMenuItem, is_scrolling));
    printf("\r\n");
}
