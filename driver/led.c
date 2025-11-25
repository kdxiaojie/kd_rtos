#include "stm32f4xx.h"
#include <stdio.h>
#include <stdint.h>
#include "cpu_tick.h"
#include "led.h"

void LED_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOB, ENABLE);

    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_OUT;
    GPIO_InitStructure.GPIO_OType = GPIO_OType_PP;
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_NOPULL;
    GPIO_Init(GPIOB, &GPIO_InitStructure);

    GPIO_SetBits(GPIOB, GPIO_Pin_0); // 默认灭
}

// 任务1：亮灯
void led_on(void)
{
    int a = 1;
    while(1) // !修正1：rtos中task的函数必须是死循环
    {
        GPIO_ResetBits(GPIOB, GPIO_Pin_0); // 亮灯 (PB0)
        printf("times_on:%d\n",a++);

        cpu_delay_ms(1000);

        // 因为我们还没写SysTick中断调度，必须手动触发 PendSV
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
}

// 任务2：灭灯
void led_off(void)
{
    int a = 1;
    while(1)
    {
        GPIO_SetBits(GPIOB, GPIO_Pin_0); // 灭灯 (PB0)
        printf("times_off:%d\n",a++);

        cpu_delay_ms(1000);

        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }
}

