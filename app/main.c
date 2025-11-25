#include <stdint.h>
#include <stdio.h>
#include "switch.h"
#include "stm32f4xx.h"
#include "cpu_tick.h"
#include "usart.h"
#include "led.h"

// 声明外部函数，方便调用
extern void os_start(void);
extern task_tcb *current_tcb;

// 定义任务句柄
task_tcb* task1;
task_tcb* task2;


int main(void)
{
    // 1. 硬件初始化
    usart_init();
    LED_Init();
    cpu_tick_init(); // 极其重要：初始化时钟，否则 delay 函数会卡死

    // 2. 创建任务
    task1 = task_create(led_on, 1024, "led_on");
    task2 = task_create(led_off, 1024, "led_off");

    // 3. 调度器启动前的准备
    // 告诉系统，第一个运行的是 task1
    current_tcb = task1;

    // 4. 启动内核！
    // CPU 将跳入 led_on 函数，永远不再返回这里
    os_start();

    while(1)
    {
        // 如果程序跑到了这里，说明出大问题了（os_start 失败）
    }
}
