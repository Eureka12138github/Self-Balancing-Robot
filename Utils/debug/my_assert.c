// my_assert.c
#include "my_assert.h"                                                 

void my_assert_handler(const char* expr, const char* file, int line) {
    (void)expr; (void)file; (void)line;

    // Keil 调试断点，执行到这里后控制权交给调试器，可以继续往下面执行，但由keil端控制
    __asm { BKPT 0 }    //开发阶段：用 BKPT，方便调试；


    // 如果要执行到这里的while循环，要注释上面的 BKPT 语句
    while (1) {
//		Serial_Printf(USART_DEBUG, "\n\rASSERT FAILED: %s @ %s:%d\n", expr, file, line);
//		Delay_ms(100);
        // 可选：点亮错误 LED
        // HAL_GPIO_WritePin(LED_GPIO, LED_PIN, GPIO_PIN_SET);
    }
}

