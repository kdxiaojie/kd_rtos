#include "key.h"
#include "cpu_tick.h" // 需要用到延时函数进行消抖

// 按键初始化函数
void KEY_Init(void)
{
    GPIO_InitTypeDef GPIO_InitStructure;

    // 1. 开启 GPIOA 和 GPIOC 的时钟
    // KEY1 在 PA0，KEY2/3 在 PC4/5
    RCC_AHB1PeriphClockCmd(RCC_AHB1Periph_GPIOA | RCC_AHB1Periph_GPIOC, ENABLE);

    // 2. 配置 KEY1 (PA0)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_0;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;       // 输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_DOWN;     //TODO: 下拉 (默认低电平，按下变高)
    GPIO_Init(GPIOA, &GPIO_InitStructure);

    // 3. 配置 KEY2 (PC4) 和 KEY3 (PC5)
    GPIO_InitStructure.GPIO_Pin = GPIO_Pin_4 | GPIO_Pin_5;
    GPIO_InitStructure.GPIO_Mode = GPIO_Mode_IN;       // 输入模式
    GPIO_InitStructure.GPIO_Speed = GPIO_Speed_100MHz;
    GPIO_InitStructure.GPIO_PuPd = GPIO_PuPd_UP;       //TODO: 上拉 (默认高电平，按下变低)
    GPIO_Init(GPIOC, &GPIO_InitStructure);
}

