#ifndef __EVENT_H
#define __EVENT_H

#include "list.h" // 需要用到链表
#include "task.h"

// 1.事件类型枚举
typedef enum
{
    EVENT_TYPE_SEM,     // 信号量
    EVENT_TYPE_QUEUE,    // 队列
    EVENT_TYPE_MBOX     // !!! 新增：邮箱类型 !!!
} event_type_t;

// 2. 信号量结构体 (这就是 ECB 的一种具体形态)
typedef struct
{
    event_type_t type;      // 类型：我是二值信号量
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

// !!! 新增：邮箱结构体 !!!
typedef struct
{
    event_type_t type;      // 类型
    list_t wait_list;       // 等待列表 (等待邮件的任务)
    void *msg;              // 存放消息的指针 (只能存1个)
    uint8_t is_full;        // 状态：0=空, 1=满
} mailbox_t;

// 函数声明
sem_t* sem_create(uint32_t init_count);
void sem_delete(sem_t *sem);
void sem_take(sem_t *sem); // 获取信号
void sem_give(sem_t *sem); // 释放信号
void sem_get_info(sem_t *sem, sem_info_t *info);
uint32_t task_wait_notify(void);
void task_notify(task_tcb *target_tcb, uint32_t value);
// 邮箱函数声明
mailbox_t* mbox_create(void);
void mbox_delete(mailbox_t *mbox);
int mbox_post(mailbox_t *mbox, void *msg); // 发送 (覆盖)
void* mbox_fetch(mailbox_t *mbox);


#endif
