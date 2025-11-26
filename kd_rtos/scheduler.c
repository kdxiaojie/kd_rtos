#include "task.h"
#include "stm32f4xx.h" // 为了使用 __CLZ

// ====================================================
// 全局变量定义
// ====================================================

//*调度锁嵌套计数器：0=未锁，>0=锁定
//*volatile 防止编译器优化
volatile uint8_t OSSchedLockNesting = 0;

// 开启调度锁：禁止任务切换
void OSSchedLock(void)
{
    __disable_irq(); //!关中断是为了保护计数器本身的加减操作是原子的

    if (OSSchedLockNesting < 255) // 防止溢出
    {
        OSSchedLockNesting++;
    }

    __enable_irq();
}

// 解除调度锁
void OSSchedUnlock(void)
{
    __disable_irq();

    if (OSSchedLockNesting > 0)
    {
        OSSchedLockNesting--;

        // !!! 关键点 !!!
        // 如果减到0了，说明锁完全解开了。
        // 此时，如果在锁定期间有高优先级任务就绪，或者时间片到了，
        // 我们应该立刻尝试一次调度，不要等下一个 Tick。
        if (OSSchedLockNesting == 0)
        {
            // 手动触发 PendSV，让调度器看看是不是该换人了
            SCB->ICSR |= SCB_ICSR_PENDSVSET_Msk;
        }
    }

    __enable_irq();
}

// 就绪列表数组：ReadyList[0] 放优先级0的任务，ReadyList[31] 放优先级31的任务
list_t ReadyList[MAX_PRIORITY];

// 优先级位图：某位为1，表示该优先级有任务就绪
uint32_t PrioBitmap = 0;

// 当前运行的任务
task_tcb *current_tcb = NULL;
// 下一个要运行的任务
task_tcb *next_tcb = NULL;

// ====================================================
// 内部函数：位图操作
// ====================================================

// 标记某优先级有任务
void bitmap_set(uint32_t prio)
{
    //!移位操作尽量用无符号数
    //!1 << 31 是有符号数的禁区，但在无符号数 (uint32_t) 中
    //!它只是单纯的 0x80000000，非常安全
    if (prio < MAX_PRIORITY) {
        PrioBitmap |= (1u << prio);
    }
    else{
    PrioBitmap |= (1u << (MAX_PRIORITY - 1));
    }
}

// 标记某优先级无任务
void bitmap_clear(uint32_t prio)
{
    if (prio < MAX_PRIORITY) {
        PrioBitmap &= ~(1u << prio);
    }
}

// 查找最高优先级 (数字越大优先级越高)
// 我们这里采用：31 最高，0 最低 (符合 bit 位置习惯)
uint32_t get_highest_priority(void)
{
    // __CLZ 返回前导零的个数。
    // 如果 PrioBitmap = 0x80000000 (最高位是1)，__CLZ 返回 0。   31 - 0 = 31。
    // 如果 PrioBitmap = 0x00000001 (最低位是1)，__CLZ 返回 31。  31 - 31 = 0。
    
    if (PrioBitmap == 0) return 0; // 没有任务
    return 31 - __CLZ(PrioBitmap);
}

// ====================================================
// 核心：调度算法
// ====================================================
void switch_context_logic(void)
{
    // 1. 查找最高优先级
    uint32_t highest_prio = get_highest_priority();

    // 2. 获取该优先级列表
    list_t *target_list = &ReadyList[highest_prio];
    list_node_t *node = target_list->head;
    
    if (node != NULL)
    {
        // 正常情况：找到了最高优先级的任务
        next_tcb = (task_tcb *)(node->owner_tcb);
        // !!! 核心修改：实现时间片轮转 (Round-Robin) !!!
        target_list->head = node->next;
    }
    else
    {
        // !!! 异常情况 !!!
        // 理论上如果有空闲任务，这里绝对不会进来。
        // 但如果还没创建空闲任务，这里必须处理，否则 next_tcb 是脏数据。
        
        // 临时方案：如果没任务跑，就让当前任务继续跑（防止指针乱指）
        // 正式方案：必须由 Idle Task 来填补这里
        next_tcb = current_tcb;
    }
}
