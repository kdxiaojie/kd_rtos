#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include "switch.h"

task_tcb* task_create(void *task_function,uint32_t task_stack_depth,char *task_name)
{
    //*申请内存，否则new_task_tcb是野指针
    task_tcb *new_task_tcb = (task_tcb *)malloc(sizeof(task_tcb));
    if (new_task_tcb == NULL)
    {
        return NULL;
    }

    //*申请栈空间
    uint32_t *stack_start = (uint32_t *)malloc(task_stack_depth * sizeof(uint32_t));
    if (stack_start == NULL)
    {
        free(new_task_tcb);
        return NULL;
    }

    //*向下增长：栈指针初始化到最高地址 (栈底)
    new_task_tcb->stack_ptr = stack_start + task_stack_depth;

    new_task_tcb->task_function = task_function;
    new_task_tcb->task_stack_depth = task_stack_depth;
    new_task_tcb->task_name = task_name;

    return new_task_tcb;
}
