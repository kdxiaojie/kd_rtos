#include <stdint.h>
#include "switch.h"
#include "stm32f4xx.h"
#include "switch.h"
#include "cpu_tick.h"

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

    GPIO_SetBits(GPIOB, GPIO_Pin_0);
}

void led_on(void)
{
	GPIO_ResetBits(GPIOB, GPIO_Pin_0);
}

void led_off(void)
{
	GPIO_SetBits(GPIOB, GPIO_Pin_0);
}

int main(void)
{
	LED_Init();
    cpu_tick_init();
	task_tcb* task1 = task_create(led_on,1024,"led_on");
	task_tcb* task2 = task_create(led_off,1024,"led_off");

	while(1)
	{
		((void(*)(void))task1->task_function)();
        cpu_delay_ms(1000);

        ((void(*)(void))task2->task_function)();
        cpu_delay_ms(1000);
	}
}
