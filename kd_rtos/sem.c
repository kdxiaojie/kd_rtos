#include "stm32f4xx.h"
#include "event.h"
#include "scheduler.h"

// 初始化
void sem_init(sem_t *sem, uint32_t init_count)
{
    if (sem == NULL) return;
    sem->type = EVENT_TYPE_SEM;
    sem->counter = init_count;
    list_init(&sem->wait_list);
}

// ============================================================
// sem_take (原 sem_wait)
// 含义：试图"拿走"一个资源。拿不到就睡觉。
// ============================================================
void sem_take(sem_t *sem)
{
    // 防御：如果调度器被锁了，严禁调用阻塞函数，否则死锁
    if (OSSchedLockNesting > 0) return;

    __disable_irq();

    // --- 情况A：有资源，直接拿走 ---
    if (sem->counter > 0)
    {
        sem->counter--;
    }
    // --- 情况B：没资源，去睡觉 ---
    else
    {
        // 1. 从就绪列表移除
        list_remove(&ReadyList[current_tcb->task_priority], &current_tcb->status_node);

        // 2. 更新位图
        if (ReadyList[current_tcb->task_priority].head == NULL)
        {
            bitmap_clear(current_tcb->task_priority);
        }

        // 3. 挂入信号量等待列表
        list_insert_end(&sem->wait_list, &current_tcb->status_node);

        // 4. 触发调度
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    }

    __enable_irq();
}

// ============================================================
// sem_give (原 sem_signal)
// 含义："给予"一个资源。有人等就给那个人，没人等就放回去。
// ============================================================
void sem_give(sem_t *sem)
{
    __disable_irq();

    // --- 情况A：有人在排队，直接给他 (唤醒) ---
    if (sem->wait_list.head != NULL)
    {
        // 1. 取出排队的任务
        list_node_t *node = sem->wait_list.head;
        task_tcb *tcb = (task_tcb *)(node->owner_tcb);

        // 2. 从等待列表移除
        list_remove(&sem->wait_list, node);

        // 3. 放入就绪列表
        list_insert_end(&ReadyList[tcb->task_priority], node);

        // 4. 恢复位图
        bitmap_set(tcb->task_priority);

        // 5. 触发调度 (抢占)
        if (OSSchedLockNesting == 0)
        {
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        }
    }
    // --- 情况B：没人等，库存+1 ---
    else
    {
        sem->counter++;
        // 如果是二值信号量，可以在这里限制：
        if (sem->counter > 1) sem->counter = 1;
    }

    __enable_irq();
}
