#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "task.h"
#include "scheduler.h"
#include "stm32f4xx.h"


extern list_t ReadyList[MAX_PRIORITY];
extern list_t DelayedList;
extern task_tcb *current_tcb;

/*----------------------------------------------------------------*/
/* 宏定义与全局变量                                               */
/*----------------------------------------------------------------*/

// xPSR.T = 1 (第24位)，指示 CPU 工作在 Thumb 状态，否则 HardFault
#define XPSR_T_BIT ((uint32_t)0x01000000L)

// 外部任务句柄声明 (临时硬编码调度逻辑)
extern task_tcb *task1;
extern task_tcb *task2;

// 函数原型声明
uint32_t *task_stack_init(void *task_function, uint32_t *stack_top);


/*----------------------------------------------------------------*/
/* 任务创建与栈初始化                                             */
/*----------------------------------------------------------------*/

/**
 * @brief  初始化任务栈 (伪造现场)
 * @param  task_function 任务函数入口地址
 * @param  stack_top     栈底地址 (高地址)
 * @return uint32_t* 初始化后的栈顶指针 (SP)
 * @note   栈结构需符合 Cortex-M4 异常帧标准 + R4-R11 软件保存帧
 */
uint32_t* task_stack_init(void *task_function, uint32_t *stack_top)
{
    uint32_t *sp = stack_top;

    /* --- 1. 硬件自动压栈部分 (Exception Frame) --- */
    *(--sp) = XPSR_T_BIT;           // xPSR
    *(--sp) = (uint32_t)task_function; // PC (任务入口)
    *(--sp) = 0x00000000;           // LR (返回地址，通常指向错误处理)
    *(--sp) = 0;                    // R12
    *(--sp) = 0;                    // R3
    *(--sp) = 0;                    // R2
    *(--sp) = 0;                    // R1
    *(--sp) = 0;                    // R0

    /* --- 2. 软件手动压栈部分 (Software Saved) --- */
    *(--sp) = 0;                    // R11
    *(--sp) = 0;                    // R10
    *(--sp) = 0;                    // R9
    *(--sp) = 0;                    // R8
    *(--sp) = 0;                    // R7
    *(--sp) = 0;                    // R6
    *(--sp) = 0;                    // R5
    *(--sp) = 0;                    // R4

    return sp; // 返回最新的栈顶指针
}

/**
 * @brief  创建任务
 * @param  task_function 任务函数指针
 * @param  task_stack_depth 栈深度 (单位：字/4字节)
 * @param  task_name 任务名称
 * @return task_tcb* 创建成功的 TCB 指针，失败返回 NULL
 */
task_tcb* task_create(void *task_function, uint32_t task_stack_depth, char *task_name,uint32_t task_priority)
{
    // 1. 申请 TCB 内存
    task_tcb *new_task_tcb = (task_tcb *)malloc(sizeof(task_tcb));
    if (new_task_tcb == NULL)
    {
        return NULL;
    }

    // 2. 申请栈内存
    uint32_t *stack_start = (uint32_t *)malloc(task_stack_depth * sizeof(uint32_t));
    if (stack_start == NULL)
    {
        free(new_task_tcb);
        return NULL;
    }

    // 3. 计算栈底地址 (Cortex-M 栈向下生长，栈底在高地址)
    uint32_t *stack_top_addr = stack_start + task_stack_depth;

    // 4. 初始化栈空间，并将新的 SP 保存到 TCB
    new_task_tcb->stack_ptr = task_stack_init(task_function, stack_top_addr);

    // 5. 填充 TCB 其他信息
    new_task_tcb->task_function = task_function;
    new_task_tcb->task_stack_depth = task_stack_depth;
    new_task_tcb->task_name = task_name;
    new_task_tcb->task_priority = task_priority;

    // ============================================================
    // !!! 核心补全部分 (Step 2 调度的关键) !!!
    // ============================================================

    // 6. 初始化链表节点 (认主)
    // 这一步至关重要！调度器遍历链表时，通过 .owner 才能找到 task_tcb
    new_task_tcb->status_node.next= NULL;
    new_task_tcb->status_node.prev = NULL;
    new_task_tcb->status_node.owner_tcb = (void *)new_task_tcb;

    // 7. 将任务插入就绪列表 (入队)
    // 将任务挂载到对应优先级的 ReadyList 末尾
    // 注意：你需要实现 list_insert_end 函数
    list_insert_end(&ReadyList[task_priority], &new_task_tcb->status_node);

    // 8. 更新优先级位图 (登记)
    // 告诉调度器：这个优先级有任务了，下次可以调度它
    bitmap_set(task_priority);

    return new_task_tcb;
}

// 阻塞延时函数
void os_delay(uint32_t ticks)
{
    if (ticks == 0) return;

    // !!! 核心防御：如果调度器被锁了，禁用 os_delay !!!
    // !否则任务会把自己挂起，但又切不走，导致系统逻辑崩溃
    if (OSSchedLockNesting > 0)
    {
        return;
    }

    // 1. 关中断保护 (涉及链表移位，必须原子操作)
    __disable_irq();

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

    // 6. 开中断
    __enable_irq();

    // 7. 触发调度 (我不跑了，快找别人跑)
    SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
}

// ============================================================
// 发送任务通知 (Give / Send)
// 直接把值写入目标任务的 TCB，并唤醒它
// ============================================================
void task_notify(task_tcb *target_tcb, uint32_t value)
{
    if (target_tcb == NULL) return;

    __disable_irq();

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

    __enable_irq();
}

// ============================================================
// 等待任务通知 (Take / Wait)
// 死等，直到有人给我发通知
// ============================================================
uint32_t task_wait_notify(void)
{
    uint32_t val = 0;

    if (OSSchedLockNesting > 0) return 0; // 锁住不能等

    __disable_irq();

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
        __enable_irq(); // 切走前开中断

        // -----------------------------------------------
        // 任务在这里被切走......
        //直到 os_task_notify 把它加回 ReadyList，它才会回来
        // -----------------------------------------------
        // 4. 醒来后 (肯定是有信了)
        __disable_irq();
        val = current_tcb->notify_value;     // 拿信
        current_tcb->notify_state = NOTIFY_NONE; // 复位
    }

    __enable_irq();
    return val;
}
