#ifndef __TASK_H__
#define __TASK_H__

#include <stdint.h>
#include "list.h"

//*定义最大优先级 (比如 32，对应一个 uint32_t 的位图)
#define MAX_PRIORITY  32

typedef struct
{
    //! 栈指针必须是TCB的第一个成员，这在汇编切换上下文时能极大简化代码
    uint32_t *stack_ptr;
    uint32_t task_priority;
    uint32_t task_stack_depth;
    list_node_t status_node;
    void *task_function;
    char *task_name;
    // !!! 新增：延时计数器 (闹钟) !!!
    uint32_t delay_ticks;

}task_tcb;

task_tcb* task_create(void *task_function, uint32_t task_stack_depth, char *task_name,uint32_t task_priority);
void bitmap_set(uint32_t prio);

// 启动调度器
void start_scheduler(void);

// 延时函数
void os_delay(uint32_t ticks);

#endif /* __TASK_H__ */
