#include "stm32f4xx.h"
#include "event.h"
#include "list.h"
#include "scheduler.h"

extern list_t ReadyList[MAX_PRIORITY];
// ============================================================
// 发送任务通知 (Give / Send)
// 直接把值写入目标任务的 TCB，并唤醒它
// ============================================================
void task_notify(task_tcb *target_tcb, uint32_t value)
{
    if (target_tcb == NULL) return;

    task_enter_critical();

    // 1. 写入数据 (这里演示最简单的覆盖模式)
    target_tcb->notify_value = value;

    // 2. 标记状态：信箱有信了
    // 无论对方是否在等，先把信放进去，把旗子竖起来
    // 如果对方之前处于 NOTIFY_NONE，现在变成 NOTIFY_PENDING
    // 如果对方之前处于 NOTIFY_WAITING，下面会把它变成 NOTIFY_PENDING 并唤醒

    // 3. 检查目标任务是否正在傻等这个通知？
    if (target_tcb->notify_state == NOTIFY_WAITING)
    {
        // 目标任务正在睡觉等待，必须把它叫醒！
        // A. 状态改为：有信了 (Pending)
        target_tcb->notify_state = NOTIFY_PENDING;

        // B. 把它放入就绪列表 (因为它在等通知，肯定不在就绪表里)
        // 注意：这里假设任务是无限等待(没在DelayedList)，如果是带超时的，
        // 还需要先从 DelayedList 移除。这里简化处理，只做无限等待唤醒。
        list_insert_end(&ReadyList[target_tcb->task_priority], &target_tcb->status_node);

        // C. 恢复位图
        bitmap_set(target_tcb->task_priority);

        // D. 触发调度 (抢占)
        if (OSSchedLockNesting == 0)
        {
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        }
    }
    else
    {
        // 对方没在等，或者正在运行，那就只把状态标记为“有信”
        target_tcb->notify_state = NOTIFY_PENDING;
    }

    task_exit_critical();
}

// ============================================================
// 等待任务通知 (Take / Wait)
// 死等，直到有人给我发通知
// ============================================================
uint32_t task_wait_notify(void)
{
    uint32_t val = 0;

    if (OSSchedLockNesting > 0) return 0; // 锁住不能等

    task_enter_critical();

    // --- 情况 A: 信箱里已经有信了 ---
    if (current_tcb->notify_state == NOTIFY_PENDING)
    {
        val = current_tcb->notify_value;    // 拿信
        current_tcb->notify_state = NOTIFY_NONE; // 清空标志
    }
    // --- 情况 B: 信箱空的，睡觉去 ---
    else
    {
        // 1. 标记自己状态：我在等信
        current_tcb->notify_state = NOTIFY_WAITING;

        // 2. 从就绪列表移除 (摘牌)
        list_remove(&ReadyList[current_tcb->task_priority], &current_tcb->status_node);
        if (ReadyList[current_tcb->task_priority].head == NULL)
        {
            bitmap_clear(current_tcb->task_priority);
        }

        // 注意：这里我们不挂入 DelayedList，也不挂入任何 list。
        // 我们变成了“孤魂野鬼”，只有持有我们 TCB 指针的发信人能救我们回来。
        // (这就是为什么叫 Direct to Task)

        // 3. 触发调度
        SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        task_exit_critical(); // 切走前开中断

        // -----------------------------------------------
        // 任务在这里被切走......
        //直到 os_task_notify 把它加回 ReadyList，它才会回来
        // -----------------------------------------------
        // 4. 醒来后 (肯定是有信了)
        task_enter_critical();
        val = current_tcb->notify_value;     // 拿信
        current_tcb->notify_state = NOTIFY_NONE; // 复位
    }

    task_exit_critical();
    return val;
}
