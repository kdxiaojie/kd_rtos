#ifndef __EVENT_H
#define __EVENT_H

#include "list.h" // 需要用到链表
#include "task.h"

// 1.事件类型枚举
typedef enum
{
    EVENT_TYPE_SEM,     // 信号量
    EVENT_TYPE_QUEUE    // 队列
} event_type_t;

// 2. 信号量结构体 (这就是 ECB 的一种具体形态)
typedef struct
{
    event_type_t type;      // 类型：我是二值信号量

    // !!! 核心：等待列表 (Wait List) !!!
    // 所有想要这个信号量但没拿到的任务，都会挂在这个链表上睡觉
    list_t wait_list;

    // 计数器：对于二值信号量，只能是 0 或 1
    uint32_t counter;
} sem_t;

// 3. 定义信号量状态信息结构体
typedef struct
{
    uint32_t current_count;  // 当前剩余资源数
    uint32_t waiting_tasks;  // 当前正在排队等待的任务数量
} sem_info_t;

// 函数声明
sem_t* sem_create(uint32_t init_count);
void sem_delete(sem_t *sem);
void sem_take(sem_t *sem); // 获取信号
void sem_give(sem_t *sem); // 释放信号
void sem_get_info(sem_t *sem, sem_info_t *info);

#endif
