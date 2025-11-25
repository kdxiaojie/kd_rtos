#include "task.h"
#include "stm32f4xx.h" // 为了使用 __CLZ

// ====================================================
// 全局变量定义
// ====================================================

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
    if (prio < MAX_PRIORITY) {
        PrioBitmap |= (1 << prio);
    }
}

// 标记某优先级无任务
void bitmap_clear(uint32_t prio)
{
    if (prio < MAX_PRIORITY) {
        PrioBitmap &= ~(1 << prio);
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
