#include <stdlib.h>
#include "stm32f4xx.h"
#include "task.h"
#include "scheduler.h"
#include "cpu_tick.h"


extern list_t ReadyList[MAX_PRIORITY];

// 阻塞延时函数
void os_delay(uint32_t ticks)
{
    if (ticks == 0) return;

    // !!! 核心防御：如果调度器被锁了，禁用 os_delay !!!
    // !否则任务会把自己挂起，但又切不走，导致系统逻辑崩溃
    if (OSSchedLockNesting > 0)
    {
        cpu_delay_ms(ticks);
        return;
    }

    // 1. 进入临界区
    task_enter_critical();

    // 2. 设置闹钟
    current_tcb->delay_ticks = ticks;

    // 3. 从就绪列表中移除
    list_remove(&ReadyList[current_tcb->task_priority], &current_tcb->status_node);

    // 4. 如果该优先级没任务了，清除位图 (关键！)
    if (ReadyList[current_tcb->task_priority].head == NULL)
    {
        bitmap_clear(current_tcb->task_priority);
    }

    // 5. 加入延时列表 (挂起)
    list_insert_end(&DelayedList, &current_tcb->status_node);

    //6. 退出临界区
    task_exit_critical();

    // 7. 触发调度 (我不跑了，快找别人跑)
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}
