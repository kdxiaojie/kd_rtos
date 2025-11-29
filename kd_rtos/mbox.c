#include <stdlib.h>
#include "stm32f4xx.h"
#include "scheduler.h"
#include "event.h"

extern list_t ReadyList[MAX_PRIORITY];

// 1. 创建邮箱
mailbox_t* mbox_create(void)
{
    mailbox_t *mbox = (mailbox_t *)malloc(sizeof(mailbox_t));
    if (mbox == NULL) return NULL;

    mbox->type = EVENT_TYPE_MBOX;
    list_init(&mbox->wait_list);
    mbox->msg = NULL;
    mbox->is_full = 0; // 初始为空

    return mbox;
}

// 2. 删除邮箱
void mbox_delete(mailbox_t *mbox)
{
    if (mbox == NULL) return;

    task_enter_critical();
    // 清场：唤醒所有等待的任务
    while (mbox->wait_list.head != NULL)
    {
        list_node_t *node = mbox->wait_list.head;
        task_tcb *tcb = (task_tcb *)(node->owner_tcb);
        list_remove(&mbox->wait_list, node);
        list_insert_end(&ReadyList[tcb->task_priority], node);
        bitmap_set(tcb->task_priority);
    }
    free(mbox);

    if (OSSchedLockNesting == 0) SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
    task_exit_critical();
}

// 3. 发送邮件 (Post)
// 特点：始终成功。如果满了，直接覆盖旧数据 (Overwrite)。
int mbox_post(mailbox_t *mbox, void *msg)
{
    if (mbox == NULL) return -1;

    task_enter_critical();

    // A. 存入数据 (覆盖)
    mbox->msg = msg;
    mbox->is_full = 1;

    // B. 检查有没有人在等信
    if (mbox->wait_list.head != NULL)
    {
        // 唤醒第一个等待的任务
        list_node_t *node = mbox->wait_list.head;
        task_tcb *tcb = (task_tcb *)(node->owner_tcb);

        list_remove(&mbox->wait_list, node);
        list_insert_end(&ReadyList[tcb->task_priority], node);
        bitmap_set(tcb->task_priority);

        // 触发调度
        if (OSSchedLockNesting == 0)
        {
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        }
    }

    task_exit_critical();
    return 0;
}

// 4. 收取邮件 (Fetch)
// 如果有信，拿走并清空信箱；没信，睡觉。
void* mbox_fetch(mailbox_t *mbox)
{
    void *return_msg = NULL;

    if (mbox == NULL) return NULL;
    if (OSSchedLockNesting > 0) return NULL; // 锁住不能阻塞

    task_enter_critical();

    // --- 情况A: 没信，睡觉 ---
    // 使用 while 是为了防止“虚假唤醒”或者被唤醒后信又被别人抢了(虽然这里是FIFO)
    while (mbox->is_full == 0)
    {
        // 1. 移除就绪
        list_remove(&ReadyList[current_tcb->task_priority], &current_tcb->status_node);
        if (ReadyList[current_tcb->task_priority].head == NULL)
        {
            bitmap_clear(current_tcb->task_priority);
        }

        // 2. 加入等待列表
        list_insert_end(&mbox->wait_list, &current_tcb->status_node);

        // 3. 调度
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        task_exit_critical();

        // ... 任务在这里切走 ...
        // ... 等待 mbox_post 唤醒 ...

        task_enter_critical();
    }

// --- 情况B: 有信了 (或者醒来了) ---
return_msg = mbox->msg;
mbox->is_full = 0; // 取走了，信箱变空

task_exit_critical();
return return_msg;
}
