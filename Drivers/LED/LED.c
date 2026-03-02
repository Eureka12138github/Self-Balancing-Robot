#include "led.h"

void LED_Init(void)
{
    RCC_APB2PeriphClockCmd(LED_RCC_APB2Periph, ENABLE);
    GPIO_InitTypeDef gpio;
    gpio.GPIO_Mode = GPIO_Mode_Out_PP;
    gpio.GPIO_Speed = GPIO_Speed_50MHz;
    gpio.GPIO_Pin = LED1_PIN | LED2_PIN;
    GPIO_Init(LED_GPIO_PORT, &gpio);
    
    LED1_OFF();
    LED2_OFF();
}
