#include "list.h"

// 初始化链表
void list_init(list_t *list)
{
    list->head = NULL;
    list->count = 0;
}

// 将节点插入到链表末尾 (用于同优先级的时间片轮转)
void list_insert_end(list_t *list, list_node_t *node)
{
    // 如果链表为空
    if (list->head == NULL)
    {
        list->head = node;
        node->next = node; // 自己指向自己 (循环)
        node->prev = node;
    }
    else
    {
        list_node_t *first = list->head;
        list_node_t *last = first->prev;

        // 插入到 last 和 first 之间
        node->next = first;
        node->prev = last;
        
        first->prev = node;
        last->next = node;
    }
    list->count++;
}

// 从链表中移除节点
void list_remove(list_t *list, list_node_t *node)
{
    if (list->head == NULL || list->count == 0) return;

    // 如果只有一个节点
    if (node->next == node)
    {
        list->head = NULL;
    }
    else
    {
        node->prev->next = node->next;
        node->next->prev = node->prev;
        
        // 如果删除的是头节点，头指针要移到下一个
        if (list->head == node)
        {
            list->head = node->next;
        }
    }
    list->count--;
}
