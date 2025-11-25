#ifndef __LIST_H
#define __LIST_H

#include <stdint.h>
#include <stddef.h>

// 定义链表节点结构体
typedef struct list_node
{
    struct list_node *next; // 指向下一个节点
    struct list_node *prev; // 指向上一个节点
    void *owner_tcb;        // 指向所属的TCB (方便通过节点找到任务)
} list_node_t;

// 定义链表头结构体 (用来管理一串任务)
typedef struct
{
    list_node_t *head;      // 头节点
    uint32_t count;         // 节点数量
} list_t;

// 函数声明
void list_init(list_t *list);
void list_insert_end(list_t *list, list_node_t *node);
void list_remove(list_t *list, list_node_t *node);

#endif
