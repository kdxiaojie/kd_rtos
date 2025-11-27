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

mailbox_t *mbox_led1;
mailbox_t *mbox_led2;

void task_led1_func(void)
{
    char * task1_context;
    printf("task1:off\n");
    task1_context = mbox_fetch(mbox_led1);
    printf("task1:on\n");
    printf("[task1]receive:%s",task1_context);
    while(1)
    {
        GPIO_ResetBits(GPIOB,GPIO_Pin_0);
        os_delay(400);
        GPIO_SetBits(GPIOB,GPIO_Pin_0);
        os_delay(400);
    }
}

void task_led2_func(void)
{
    char * task2_context;
    printf("task2:off\n");
    task2_context = mbox_fetch(mbox_led2);
    printf("task2:on\n");
    printf("[task2]receive:%s",task2_context);
    while(1)
    {
        GPIO_ResetBits(GPIOB,GPIO_Pin_1);
        os_delay(400);
        GPIO_SetBits(GPIOB,GPIO_Pin_1);
        os_delay(400);
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
    char * task1_context = "The first led is working.";
    char * task2_context = "The second led is working.";
    while(1)
    {
        if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) // 按下
        {
            os_delay(20);
            if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0)
            {
                printf("key1 press");
                // !!! 发送邮箱给 task_led1 !!!
                mbox_post(mbox_led1,task1_context);
                while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) cpu_delay_ms(10);
            }
        }

        if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == 0) // 按下
        {
            os_delay(20);
            if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == 0)
            {
                printf("key2 press");
                // !!! 发送邮箱给 task_led2 !!!
                mbox_post(mbox_led2,task2_context);
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

    printf("\n=== MH-RTOS Priority Scheduling Test ===\n");

    printf("create mbox_led1\n");
    printf("create mbox_led2\n");
    mbox_led1 = mbox_create();
    mbox_led2 = mbox_create();

    // 2. 创建任务 (注意优先级参数)
    // 参数：函数，栈大小，名字，优先级
    task_key  = task_create(task_key_func, 1024, "KeyTask", 3);
    task_led1 = task_create(task_led1_func, 1024, "High", 2);
    task_led2 = task_create(task_led2_func,  1024, "Low",  2);
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

