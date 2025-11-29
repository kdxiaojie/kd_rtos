#ifndef __TASK_H__
#define __TASK_H__

#include <stdint.h>
#include "list.h"

// 全局临界区嵌套计数器
extern volatile uint32_t critical_nesting;

// 临界区管理函数
void task_enter_critical(void);
void task_exit_critical(void);

// 定义通知状态
#define NOTIFY_NONE     0 // 无通知，也没在等
#define NOTIFY_PENDING  1 // 有通知了 (信箱有信)
#define NOTIFY_WAITING  2 // 正在死等通知 (人在睡觉)

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

    // !!! 新增：任务通知专用字段 !!!
    uint32_t notify_value;  // 私人信箱 (存数值)
    uint8_t  notify_state;  // 信箱状态 (有没有信，或者人是否在等)
}task_tcb;

task_tcb* task_create(void *task_function, uint32_t task_stack_depth, char *task_name,uint32_t task_priority);
void bitmap_set(uint32_t prio);

// 启动调度器
void start_scheduler(void);

// 延时函数
void os_delay(uint32_t ticks);

#endif /* __TASK_H__ */
