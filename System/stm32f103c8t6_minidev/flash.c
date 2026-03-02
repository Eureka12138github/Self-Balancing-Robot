#include "flash.h"

/**
 * @brief 从指定 Flash 地址读取一个 32 位字（4 字节）
 * @param[in] Address 读取地址（必须对齐到 4 字节边界）
 * @return 读取到的 32 位数据
 * @note 地址应位于有效 Flash 区域，且无需解锁即可读取。
 */
u32 MyFLASH_ReadWord(u32 Address)
{
    return *((__IO u32 *)(Address));
}

/**
 * @brief 从指定 Flash 地址读取一个 16 位半字（2 字节）
 * @param[in] Address 读取地址（必须对齐到 2 字节边界）
 * @return 读取到的 16 位数据
 * @note 地址应位于有效 Flash 区域。
 */
u16 MyFLASH_ReadHalfWord(u32 Address)
{
    return *((__IO u16 *)(Address));
}

/**
 * @brief 从指定 Flash 地址读取一个 8 位字节
 * @param[in] Address 读取地址
 * @return 读取到的 8 位数据
 * @note 支持任意字节对齐地址。
 */
u8 MyFLASH_ReadByte(u32 Address)
{
    return *((volatile u8 *)(Address));
}

/**
 * @brief 擦除整个 Flash（包括程序代码区）
 * @warning 此操作将擦除全部 Flash 内容，导致程序无法继续运行！
 *          通常仅用于安全自毁、防拆机等特殊场景。
 * @note 调用后系统将处于不可恢复状态，需谨慎使用。
 */
void MyFLASH_EraseAllPages(void)
{
    FLASH_Unlock();
    FLASH_EraseAllPages();
    FLASH_Lock();
}

/**
 * @brief 擦除 Flash 中指定页
 * @param[in] Page_Address 页起始地址（如 0x08000000, 0x08000400 等）
 * @note 地址必须为页对齐（STM32F1/F4 等常见页大小为 1KB/2KB）。
 *       擦除前会自动解锁 Flash，操作完成后自动上锁。
 */
void MyFLASH_ErasePage(u32 Page_Address)
{
    FLASH_Unlock();
    FLASH_ErasePage(Page_Address);    
    FLASH_Lock();
}

/**
 * @brief 向 Flash 指定地址写入一个 32 位字
 * @param[in] Address 写入地址（必须对齐到 4 字节，且所在页已擦除）
 * @param[in] Data    要写入的 32 位数据
 * @note 目标地址所在页必须先擦除（值为 0xFFFFFFFF），否则写入无效。
 *       函数内部自动处理 Flash 解锁与上锁。
 */
void MyFLASH_ProgramWord(u32 Address, u32 Data)
{
    FLASH_Unlock();
    FLASH_ProgramWord(Address, Data);
    FLASH_Lock();
}

/**
 * @brief 向 Flash 指定地址写入一个 16 位半字
 * @param[in] Address 写入地址（必须对齐到 2 字节，且所在页已擦除）
 * @param[in] Data    要写入的 16 位数据
 * @note 目标地址所在页必须先擦除。函数自动管理 Flash 锁状态。
 */
void MyFLASH_ProgramHalfWord(u32 Address, u16 Data)
{
    FLASH_Unlock();
    FLASH_ProgramHalfWord(Address, Data);
    FLASH_Lock();
}
