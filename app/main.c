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
        os_delay(300);

        GPIO_SetBits(GPIOB, GPIO_Pin_0);   // 灭灯

        os_delay(3000);
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
        os_delay(1000);
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
    while(1)
    {
        // ============================================================
        // KEY1 (PA0): 加锁测试
        // ============================================================
        if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0)
        {
            // !!! 修正点 1：智能消抖 !!!
            // 如果已经锁了，绝对不能调 os_delay，否则回不来！
            if(OSSchedLockNesting == 0)
                os_delay(20);      // 没锁：用阻塞延时，让出CPU
            else
                cpu_delay_ms(20);  // 锁了：只能用死延时霸占CPU

            // 确认按下
            if(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0)
            {
                printf("\n>>> KEY1: Lock++\n");
                OSSchedLock(); // 加锁
                printf("Lock Count: %d\n", OSSchedLockNesting);

                // 等待松开 (死等)
                while(GPIO_ReadInputDataBit(GPIOA, GPIO_Pin_0) == 0) 
                {
                    cpu_delay_ms(10); 
                }
            }
        }

        // ============================================================
        // KEY2 (PC4): 解锁测试
        // ============================================================
        if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == 0)
        {
            // 解锁前肯定是锁着的（或者没锁），为了安全，直接用死延时消抖
            // 因为这里 os_delay 风险太高
            cpu_delay_ms(20); 
            
            if(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == 0)
            {
                // 只有锁大于0才解，防止减到负数溢出
                if (OSSchedLockNesting > 0)
                {
                    printf("\n<<< KEY2: Unlock--\n");
                    OSSchedUnlock(); 
                    printf("Lock Count: %d\n", OSSchedLockNesting);
                }
                else
                {
                    printf("Ignored. Not Locked.\n");
                }

                // 等待松开
                while(GPIO_ReadInputDataBit(GPIOC, GPIO_Pin_4) == 0) 
                {
                    cpu_delay_ms(10);
                }
            }
        }

        // ============================================================
        // 循环间歇
        // ============================================================
        if (OSSchedLockNesting > 0)
        {
            // ！！！修正点 2：锁定期间必须勤快检查！！！
            // 如果用 100ms 甚至更久，你会感觉按键“按不动”（因为CPU在睡觉）
            // 建议 10ms - 20ms
            cpu_delay_ms(10); 
        }
        else
        {
            // 没锁的时候，正常睡觉，把 CPU 让给别人
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

