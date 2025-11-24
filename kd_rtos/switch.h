#ifndef __SWITCH_H__
#define __SWITCH_H__

#include <stdint.h>

typedef struct
{
    //! 栈指针必须是TCB的第一个成员，这在汇编切换上下文时能极大简化代码
    uint32_t *stack_ptr;
    void *task_function;
    uint32_t task_stack_depth;
    char *task_name;
}task_tcb;

task_tcb* task_create(void *task_function,uint32_t task_stack_depth,char *task_name);

#endif /* __RTC_H__ */
