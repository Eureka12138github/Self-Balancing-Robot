// my_assert.c
#include "my_assert.h"                                                 

void my_assert_handler(const char* expr, const char* file, int line) {
    (void)expr; (void)file; (void)line;

#if defined(__CC_ARM)          // Keil MDK (ARMCC)
    __asm { BKPT 0 };          // Keil 调试断点
#elif defined(__GNUC__)        // GCC (PlatformIO)
    __asm__ volatile("BKPT #0\n");  // GCC 内联汇编，触发 HardFault
#endif

    // 如果要执行到这里的 while 循环，要注释上面的 BKPT 语句
    while (1) {
//		Serial_Printf(USART_DEBUG, "\n\rASSERT FAILED: %s @ %s:%d\n", expr, file, line);
//		Delay_ms(100);
        // 可选：点亮错误 LED
        // HAL_GPIO_WritePin(LED_GPIO, LED_PIN, GPIO_PIN_SET);
    }
}

