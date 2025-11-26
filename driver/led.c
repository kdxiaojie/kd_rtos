#include "stm32f4xx.h"
#include <stdio.h>
#include <stdint.h>
#include "cpu_tick.h"
#include "led.h"

void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOE, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0 | GPIO_Pin_1;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_9;
    GPIO_Init(GPIOE, &GPIO_InitStructure);

    //*³õÊ¼»¯Îª OFF ×´Ì¬
    GPIO_SetBits(GPIOB,GPIO_Pin_0);
    GPIO_SetBits(GPIOB,GPIO_Pin_1);
    GPIO_SetBits(GPIOE,GPIO_Pin_9);
}


