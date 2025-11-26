#include <stdint.h>
#include <stdio.h>
#include "task.h"
#include "stm32f4xx.h"
#include "cpu_tick.h"
#include "usart.h"
#include "led.h"
#include "key.h"
#include "scheduler.h"

// 声明外部函数
extern void os_start(void);
extern task_tcb *current_tcb;

// 定义任务句柄
task_tcb* task_key;
task_tcb* task_high;
task_tcb* task_low;
task_tcb* task_idle;

// -------------------------------------------------------------------
// 任务 1 (High Priority): 每 1秒 跑一次
// 优先级设为 2
// -------------------------------------------------------------------
void task_high_func(void)
{
    while(1)
    {
        GPIO_ResetBits(GPIOB, GPIO_Pin_0); // 亮灯
        printf("[HIGH] Task Running! (Prio 2)\n");
        
        // 模拟干活
        cpu_delay_ms(300);
        
        GPIO_SetBits(GPIOB, GPIO_Pin_0);   // 灭灯

        cpu_delay_ms(300);
        
        // !!! 关键 !!!
        // 这里必须让出 CPU (暂时用死延时模拟阻塞，实际上应该用 vTaskDelay)
        // 你的系统现在还没有实现阻塞延时，所以一旦这个任务跑起来，
        // 它是最高优先级，它会一直跑，低优先级的任务将永远无法运行！
        //
        // 为了演示抢占，我们这里暂时只能手动切出去，或者依靠 SysTick 中断
        // 但因为你还没有实现“阻塞态”，现在所有的任务都在就绪表中。
        // 调度器每次都会选最高优先级的任务。
        // 所以：现象将是 task_high 一直霸占 CPU。
        //cpu_delay_ms(1000);
    }
}

// -------------------------------------------------------------------
// 任务 2 (Low Priority): 优先级设为 1
// -------------------------------------------------------------------
void task_low_func(void)
{
    while(1)
    {
        printf("[LOW ] Task Running! (Prio 1)\n");
        cpu_delay_ms(1000);
    }
}


// -------------------------------------------------------------------
// 任务 3 (Priority): 优先级设为 3
// -------------------------------------------------------------------
void task_key_func(void)
{
    while(1)
    {
        if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0)
        {
            cpu_delay_ms(100);
            if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0)
            {
                OSSchedLock();
                printf("os clock:%d",OSSchedLockNesting);
            }
        }

        if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == 0)
        {
            cpu_delay_ms(100);
            if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == 0)
            {
                OSSchedUnlock();
                printf("os clock:%d",OSSchedLockNesting);
            }
        }

        if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5) == 0)
        {
            cpu_delay_ms(100);
            if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_5) == 0)
            {
                printf("key3 press");
            }
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
    // ! 0 !!! 核心修改：先关中断 !!!
    // 防止 SysTick 在 os_start 之前捣乱
    __disable_irq();

    // 1. 硬件初始化
    usart_init();
    LED_Init();
    KEY_Init();

    printf("\n=== MH-RTOS Priority Scheduling Test ===\n");

    // 2. 创建任务 (注意优先级参数)
    // 参数：函数，栈大小，名字，优先级
    task_key  = task_create(task_key_func, 1024, "KeyTask", 2);
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

