#ifndef BSP_FLASH_H
#define BSP_FLASH_H

#include "stm32f10x.h"

/**
 * @brief 从 Flash 地址读取 1 字节
 * @param Address 任意有效 Flash 地址
 * @return 读取到的字节数据
 */
u8 MyFLASH_ReadByte(u32 Address);

/**
 * @brief 从 Flash 地址读取 2 字节（半字）
 * @param Address 地址必须 2 字节对齐
 * @return 读取到的半字数据
 */
u16 MyFLASH_ReadHalfWord(u32 Address);

/**
 * @brief 从 Flash 地址读取 4 字节（字）
 * @param Address 地址必须 4 字节对齐
 * @return 读取到的字数据
 */
u32 MyFLASH_ReadWord(u32 Address);

/**
 * @brief 擦除 Flash 中指定页
 * @param Page_Address 页起始地址（如 0x08000000），必须页对齐
 */
void MyFLASH_ErasePage(u32 Page_Address);

/**
 * @brief 擦除整个 Flash（含程序区）— 极度危险！
 * @warning 调用后程序将被擦除，系统无法继续运行！
 */
void MyFLASH_EraseAllPages(void);

/**
 * @brief 向 Flash 写入 2 字节（半字）
 * @param Address 地址必须 2 字节对齐，且所在页已擦除
 * @param Data    要写入的数据
 */
void MyFLASH_ProgramHalfWord(u32 Address, u16 Data);

/**
 * @brief 向 Flash 写入 4 字节（字）
 * @param Address 地址必须 4 字节对齐，且所在页已擦除
 * @param Data    要写入的数据
 */
void MyFLASH_ProgramWord(u32 Address, u32 Data);

#endif
