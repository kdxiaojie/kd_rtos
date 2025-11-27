#include <stdint.h>
#include <stdio.h>
#include "task.h"
#include "stm32f4xx.h"
#include "cpu_tick.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "scheduler.h"
#include "event.h"
#include "sem.h"

// 声明外部函数
extern void os_start(void);
extern task_tcb *current_tcb;

// 定义任务句柄
task_tcb* task_key;
task_tcb* task_led1;
task_tcb* task_led2;
task_tcb* task_idle;

// 定义信号量
sem_t *led_sem;

// -------------------------------------------------------------------
// 任务 1 (High Priority): 每 1秒 跑一次
// 优先级设为 2
// -------------------------------------------------------------------
void task_led1_func(void)
{
    uint32_t notify_led1;
    printf("task1:off\n");
    notify_led1 = task_wait_notify();
    printf("task1:on\n");
    while(1)
    {
        GPIO_ResetBits(GPIOB,GPIO_Pin_0);
        os_delay(400);
        GPIO_SetBits(GPIOB,GPIO_Pin_0);
        os_delay(400);
        printf("task1 notify value:%d",notify_led1);
    }
}

// -------------------------------------------------------------------
// 任务 2 (Low Priority): 优先级设为 1
// -------------------------------------------------------------------
void task_led2_func(void)
{
    uint32_t notify_led2;
    printf("task2:off\n");
    notify_led2 = task_wait_notify();
    printf("task2:on\n");
    while(1)
    {
        GPIO_ResetBits(GPIOB,GPIO_Pin_1);
        os_delay(400);
        GPIO_SetBits(GPIOB,GPIO_Pin_1);
        os_delay(400);
        printf("task2 notify value:%d",notify_led2);
    }
}


// -------------------------------------------------------------------
// 任务 3 (Priority): 优先级设为 3
// -------------------------------------------------------------------
// -------------------------------------------------------------------
// 任务 3: 按键任务
// -------------------------------------------------------------------
void task_key_func(void)
{
    uint32_t count = 0;

    while(1)
    {
        if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) // 按下
        {
            os_delay(20);
            if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0)
            {
                count++;
                printf("[Key Task] Sending Notify Value: %d\n", count);
                // !!! 发送通知给 task_led1 !!!
                // 直接指定目标 TCB，不需要中间的 sem 变量
                task_notify(task_led1, count);
                while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) cpu_delay_ms(10);
            }
        }

        if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == 0) // 按下
        {
            os_delay(20);
            if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == 0)
            {
                count++;
                printf("[Key Task] Sending Notify Value: %d\n", count);
                // !!! 发送通知给 task_led1 !!!
                // 直接指定目标 TCB，不需要中间的 sem 变量
                task_notify(task_led2, count);
                while(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == 0) cpu_delay_ms(10);
            }
        }
        // ============================================================
        // 循环间歇
        // ============================================================
        if (OSSchedLockNesting > 0)
        {
            cpu_delay_ms(10);
        }
        else
        {
            os_delay(50);
        }
    }
}


// -------------------------------------------------------------------
// 空闲任务 (Idle Task): 优先级设为 0 (最低)
// -------------------------------------------------------------------
void idle_task_func(void)
{
    while(1)
    {
        // 什么都不做，或者闪烁另一个LED表示系统空闲
        // printf("."); // 打印太多会卡死，这里安静一点
    }
}

int main(void)
{
    // !!! 初始化链表 !!!
    os_init();

    // ! 0 !!! 核心修改：先关中断 !!!
    // 防止 SysTick 在 os_start 之前捣乱
    __disable_irq();

    // 1. 硬件初始化
    usart_init();
    LED_Init();
    KEY_Init();
    printf("create sem\n");
    led_sem = sem_create(0);

    printf("\n=== MH-RTOS Priority Scheduling Test ===\n");

    // 2. 创建任务 (注意优先级参数)
    // 参数：函数，栈大小，名字，优先级
    task_key  = task_create(task_key_func, 1024, "KeyTask", 3);
    task_led1 = task_create(task_led1_func, 1024, "High", 2);
    task_led2  = task_create(task_led2_func,  1024, "Low",  2);
    task_idle = task_create(idle_task_func, 256,  "Idle", 0);

    // 3. 调度器启动前的准备
    // 初始指向最高优先级任务，让它先跑
    current_tcb = task_led1;

    // !!! 在这里开启 SysTick !!!
    // !此时所有数据结构都准备好了，中断来了也能处理
    cpu_tick_init();

    // 4. 启动内核
    printf("OS Starting...\n");
    os_start();

    while(1);
}

