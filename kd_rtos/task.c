#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "task.h"

extern list_t ReadyList[MAX_PRIORITY];

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
