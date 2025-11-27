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
task_tcb* task_high;
task_tcb* task_low;
task_tcb* task_idle;

// 定义信号量
sem_t *led_sem;

// -------------------------------------------------------------------
// 任务 1 (High Priority): 每 1秒 跑一次
// 优先级设为 2
// -------------------------------------------------------------------
void task_high_func(void)
{
    printf("task1:off\n");
    sem_take(led_sem);
    printf("task1:on\n");
    while(1)
    {
        GPIO_ResetBits(GPIOB,GPIO_Pin_0);
        os_delay(400);
        GPIO_SetBits(GPIOB,GPIO_Pin_0);
        os_delay(400);
    }
}

// -------------------------------------------------------------------
// 任务 2 (Low Priority): 优先级设为 1
// -------------------------------------------------------------------
void task_low_func(void)
{
    printf("task2:off\n");
    sem_take(led_sem);
    printf("task2:on\n");
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
    sem_info_t info; // 定义一个临时变量存状态

    while(1)
    {
        // ============================================================
        // KEY1 (PA0): 释放信号量测试
        // ============================================================
        if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0)
        {
            // 如果已经锁了，绝对不能调 os_delay，否则回不来！
            if(OSSchedLockNesting == 0)
                os_delay(50);      // 没锁：用阻塞延时，让出CPU
            else
                cpu_delay_ms(50);  // 锁了：只能用死延时霸占CPU

            // 确认按下
            if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0)
            {
                printf("give sem\n");
                sem_give(led_sem);
            }
        }

        if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == 0)
        {
            // 如果已经锁了，绝对不能调 os_delay，否则回不来！
            if(OSSchedLockNesting == 0)
                os_delay(50);      // 没锁：用阻塞延时，让出CPU
            else
                cpu_delay_ms(50);  // 锁了：只能用死延时霸占CPU

            // 确认按下
            if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == 0)
            {
                printf("delete sem\n");
                sem_delete(led_sem);
            }
        }

        if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5) == 0)
        {
            // 如果已经锁了，绝对不能调 os_delay，否则回不来！
            if(OSSchedLockNesting == 0)
                os_delay(50);      // 没锁：用阻塞延时，让出CPU
            else
                cpu_delay_ms(50);  // 锁了：只能用死延时霸占CPU

            if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5) == 0)
            {
                printf("\n[Query] Checking Semaphore State...\n");
                // !!! 调用查询函数 !!!
                sem_get_info(led_sem, &info);
                // 打印结果
                printf("  - Resources Available: %d\n", info.current_count);
                printf("  - Tasks Waiting:       %d\n", info.waiting_tasks);
                while(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5) == 0) cpu_delay_ms(10);
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
    task_high = task_create(task_high_func, 1024, "High", 2);
    task_low  = task_create(task_low_func,  1024, "Low",  2);
    task_idle = task_create(idle_task_func, 256,  "Idle", 0);

    // 3. 调度器启动前的准备
    // 初始指向最高优先级任务，让它先跑
    current_tcb = task_high;

    // !!! 在这里开启 SysTick !!!
    // !此时所有数据结构都准备好了，中断来了也能处理
    cpu_tick_init();

    // 4. 启动内核
    printf("OS Starting...\n");
    os_start();

    while(1);
}

